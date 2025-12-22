#include "mainwindow.h"

#include <QApplication>
#include <QFileDialog>
#include <QStatusBar>
#include <QThread>
#include <QMessageBox>

#define DEFAULT_USERNAME    "root"              //默认用户名
#define DEFAULT_PASSWORD    "rpdzkj"            //默认密码
#define DEFAULT_PATH        "/home/rpdzkj/"     //默认工作路径

MainWindow::MainWindow(QMainWindow *parent) : QMainWindow(parent){
    setupUI();

    // 创建线程和工作对象
    m_workerThread = new QThread(this);
    m_sftpThread = new SftpThread();
    m_sftpThread->moveToThread(m_workerThread);

    // 连接工作对象的信号
    connect(m_sftpThread, &SftpThread::progress, this, &MainWindow::onProgress);
    connect(m_sftpThread, &SftpThread::fileProgress, this, &MainWindow::onFileProgress);
    connect(m_sftpThread, &SftpThread::finished, this, &MainWindow::onUploadFinished);
    connect(m_sftpThread, &SftpThread::commandExecuted, this, &MainWindow::onCommandExecuted);

    // 连接按钮信号槽
    connect(m_fileUrlBtn, &QPushButton::clicked, this, &MainWindow::onFileUrlBtnClicked);
    connect(m_startTransBtn, &QPushButton::clicked, this, &MainWindow::onStartTransBtnClicked);
    connect(m_ipBtn, &QPushButton::clicked, this, &MainWindow::onIpBtnClicked);
    connect(m_startUpdateBtn, &QPushButton::clicked, this, &MainWindow::onUpdateBtnClicked);

    // 启动线程
    m_workerThread->start();
}

MainWindow::~MainWindow() {
    m_workerThread->quit();
    m_workerThread->wait();
    delete m_sftpThread;
}

void MainWindow::setupUI() {
    //窗口
    setWindowTitle("远程更新");
    this->resize(1600, 1200);
    m_statusBar = new QStatusBar(this);
    setStatusBar(m_statusBar);

    mainWidget = new QWidget;
    this->setCentralWidget(mainWidget);

    //第一行
    m_ipLabel = new QLabel("设备IP:");
    m_curIpLabel = new QLabel("当前IP: 未配置");
    m_ipBtn = new QPushButton("配置");
    m_ipLineEdit = new QLineEdit();

    //第2行
    m_fileUrlBtn = new QPushButton("打开");
    m_fileUrlLineEdit = new QLineEdit();
    m_fileUrlLineEdit->setReadOnly(true);
    m_fileUrlLineEdit->setPlaceholderText("已选文件将显示在这里");
    m_fileUrlLabel = new QLabel("已选文件：");
    m_fileUrlLabel->setWordWrap(true);

    //第3行
    m_startTransBtn = new QPushButton("上传");
    m_startUpdateBtn = new QPushButton("更新");
    m_progressLabel = new QLabel("上传进度：");
    m_progressLabel->setWordWrap(true);
    m_progressLabel->setVisible(false);
    m_progressBar = new QProgressBar;
    m_progressBar->setVisible(false);

    //布局
    mainLayout = new QGridLayout;
    mainLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
    mainLayout->setSpacing(50);
    mainLayout->setContentsMargins(5,5,5,5);

    //第1行
    mainLayout->addWidget(m_ipLabel, 0, 0, 1, 1);
    mainLayout->addWidget(m_ipLineEdit, 0, 1, 1, 2);
    mainLayout->addWidget(m_curIpLabel, 0, 3, 1, 2);
    mainLayout->addWidget(m_ipBtn, 0, 5, 1, 1);

    //第2行
    mainLayout->addWidget(m_fileUrlLabel, 1, 0, 1, 1);
    mainLayout->addWidget(m_fileUrlLineEdit, 1, 1, 1, 4);
    mainLayout->addWidget(m_fileUrlBtn, 1, 5, 1, 1);

    //第3行
    mainLayout->addWidget(m_progressLabel, 2, 0, 1, 1);
    mainLayout->addWidget(m_progressBar, 2, 1, 1, 3);
    mainLayout->addWidget(m_startTransBtn, 2, 4, 1, 1);
    mainLayout->addWidget(m_startUpdateBtn, 2, 5, 1, 1);

    mainWidget->setLayout(mainLayout);
}

void MainWindow::onIpBtnClicked() {
    m_ip = m_ipLineEdit->text();
    m_curIpLabel->setText("当前IP: " + m_ip);
    m_statusBar->showMessage("当前IP:" + m_ip, 3000);
}

void MainWindow::onUpdateBtnClicked() {
    // 检查IP是否配置
    if (m_ip.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先配置设备IP");
        return;
    }
    const sRemoteDeviceInfo deviceInfo(m_ip,22,DEFAULT_USERNAME,DEFAULT_PASSWORD);
    QMetaObject::invokeMethod(m_sftpThread, "executeCommand",
        Qt::QueuedConnection,
        Q_ARG(sRemoteDeviceInfo, deviceInfo),
        Q_ARG(QString, "cd /home/rpdzkj && mv *.img update.img"));

    QMetaObject::invokeMethod(m_sftpThread, "executeCommand",
        Qt::QueuedConnection,
        Q_ARG(sRemoteDeviceInfo, deviceInfo),
        Q_ARG(QString, "cd /home/rpdzkj && chmod +x ./start_update.sh && ./start_update.sh"));
}


void MainWindow::onFileUrlBtnClicked() {
    // 创建文件对话框
    QStringList selectedFiles = QFileDialog::getOpenFileNames(this,
        "选择文件",
        QDir::homePath(),
        "所有文件 (*.*);;文本文件 (*.txt);;图像文件 (*.png *.jpg *.bmp);;PDF文件 (*.pdf)");

    // 检查用户是否选择了文件
    if (selectedFiles.isEmpty()) {
        m_statusBar->showMessage("已取消选择文件", 3000);
        return;
    }
    // 准备文件信息
    m_filePaths.clear();
    QString lineText = "";
    qint64 totalBytes = 0;

    for (const QString &filePath : selectedFiles) {
        QFileInfo fileInfo(filePath);
        if (!fileInfo.isFile()) continue;

        sTransFilePath path;
        path.hostPath = filePath;
        path.remotePath = DEFAULT_PATH + fileInfo.fileName();
        path.fileSize = fileInfo.size();
        path.fileName = fileInfo.fileName();

        m_filePaths.append(path);
        totalBytes += path.fileSize;

        lineText += path.fileName + ";";
    }

    if (m_filePaths.isEmpty()) {
        QMessageBox::warning(this, "警告", "未选择有效文件");
        return;
    }
    m_totalBytes = totalBytes;
    m_totalTransferredBytes = 0;
    m_totalFiles = m_filePaths.size();

    // 更新UI
    m_fileUrlLineEdit->setText(lineText);
    m_progressLabel->setText(QString("上传进度: %1/%2 文件").arg(0).arg(m_totalFiles));
}

void MainWindow::onStartTransBtnClicked(){
    // 检查是否选择了文件
    if (m_filePaths.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择要上传的文件");
        return;
    }

    // 检查IP是否配置
    if (m_ip.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先配置设备IP");
        return;
    }

    // 显示进度条
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_progressLabel->setVisible(true);
    m_progressLabel->setText(QString("上传进度: %1/%2 文件").arg(0).arg(m_totalFiles));

    // 准备参数
    const sRemoteDeviceInfo deviceInfo(m_ip,22,DEFAULT_USERNAME,DEFAULT_PASSWORD);

    // 禁用按钮防止重复点击
    m_startTransBtn->setEnabled(false);
    m_fileUrlBtn->setEnabled(false);
    m_ipBtn->setEnabled(false);

    // 使用工作线程中的工作对象执行上传
    QMetaObject::invokeMethod(m_sftpThread, "uploadFile",
        Qt::QueuedConnection,
        Q_ARG(sRemoteDeviceInfo, deviceInfo),
        Q_ARG(QVector<sTransFilePath>, m_filePaths));
}

void MainWindow::onUploadFinished(const bool success, const QString &error) const {
    // 重新启用按钮
    m_startTransBtn->setEnabled(true);
    m_fileUrlBtn->setEnabled(true);
    m_ipBtn->setEnabled(true);

    if (success) {
        m_statusBar->showMessage("所有文件上传成功！", 3000);
        m_progressLabel->setText("上传完成");
    } else {
        m_statusBar->showMessage("上传失败: " + error, 3000);
        m_progressLabel->setText("上传失败: " + error);
    }

    m_progressBar->setVisible(false);
    m_progressBar->setValue(0);
    m_progressLabel->setVisible(false);
}

void MainWindow::onProgress(const qint64 current, const qint64 total){
    updateProgressDisplay(current, total);

    // 更新状态栏信息
    if (total > 0) {
        const int percent = static_cast<int>((current * 100) / total);
        static int lastPercent = -1;

        if (percent != lastPercent && percent % 10 == 0) {
            const QString status = QString("已传输: %1/%2 MB (%3%)")
                             .arg(static_cast<double>(current) / (1024.0 * 1024.0), 0, 'f', 1)
                             .arg(static_cast<double>(total) / (1024.0 * 1024.0), 0, 'f', 1)
                             .arg(percent);
            m_statusBar->showMessage(status);
            lastPercent = percent;
        }
    }
}

void MainWindow::onFileProgress(int fileIndex, const QString &fileName, qint64 current, qint64 total)
{
    // 更新当前文件进度
    updateFileProgress(fileIndex, fileName, current, total);

    // 累计已传输字节数
    if (current > 0 && total > 0) {
        m_totalTransferredBytes += (current - (total > 0 ? current : 0)); // 避免重复计算
        updateProgressDisplay(m_totalTransferredBytes, m_totalBytes);
    }
}

void MainWindow::updateProgressDisplay(qint64 current, qint64 total)
{
    if (total <= 0) return;

    // 计算百分比
    const int percent = static_cast<int>((current * 100) / total);

    // 更新进度条
    if (percent != m_progressBar->value()) {
        m_progressBar->setValue(percent);
    }
}

void MainWindow::updateFileProgress(int fileIndex, const QString &fileName, qint64 current, qint64 total)
{
    if (fileIndex < 0 || fileIndex >= m_totalFiles) return;

    m_currentFileIndex = fileIndex;

    // 更新进度标签
    m_progressLabel->setText(QString("正在上传: %1 (%2/%3) - %4/%5MB")
        .arg(fileName)
        .arg(fileIndex + 1)
        .arg(m_totalFiles)
        .arg(static_cast<double>(current) / (1024.0 * 1024.0), 0, 'f', 1)
        .arg(static_cast<double>(total) / (1024.0 * 1024.0), 0, 'f', 1));
}

void MainWindow::onCommandExecuted(const QString &output, const QString &error)
{
    if (!error.isEmpty()) {
        m_statusBar->showMessage("命令执行错误: " + error, 3000);
    } else {
        m_statusBar->showMessage("命令执行成功", 3000);
    }
}
