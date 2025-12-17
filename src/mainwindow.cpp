#include "mainwindow.h"

#include <QApplication>
#include <QFileDialog>
#include <QStatusBar>
#include <QThread>
#include <QMessageBox>

MainWindow::MainWindow(QMainWindow *parent) : QMainWindow(parent){
    setWindowTitle("远程更新");
    this->resize(1600, 1200);
    m_statusBar = new QStatusBar(this);
    setStatusBar(m_statusBar);
    mainWidget = new QWidget;
    this->setCentralWidget(mainWidget);
    m_fileUrlBtn = new QPushButton("打开");
    m_startTransBtn = new QPushButton("上传");
    m_fileUrlLabel = new QLabel("文件路径：");
    m_fileUrlLabel->setWordWrap(true);
    m_progressLabel = new QLabel("上传进度：");
    m_progressLabel->setWordWrap(true);
    m_progressLabel->setVisible(false);
    m_fileUrlLineEdit = new QLineEdit();
    m_fileUrlLineEdit->setReadOnly(true);
    m_fileUrlLineEdit->setPlaceholderText("文件路径将显示在这里");
    m_progressBar = new QProgressBar;
    m_progressBar->setVisible(false);
    mainLayout = new QGridLayout;
    mainLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
    mainLayout->setSpacing(50);
    mainLayout->setContentsMargins(5,5,5,5);
    mainLayout->addWidget(m_fileUrlLabel, 0, 0, 1, 1);
    mainLayout->addWidget(m_fileUrlLineEdit, 0, 1, 1, 4);
    mainLayout->addWidget(m_fileUrlBtn, 0, 5, 1, 1);
    mainLayout->addWidget(m_progressLabel, 1, 0, 1, 1);
    mainLayout->addWidget(m_progressBar, 1, 1, 1, 4);
    mainLayout->addWidget(m_startTransBtn, 1, 5, 1, 1);
    mainWidget->setLayout(mainLayout);
    // 连接按钮信号槽
    connect(m_fileUrlBtn, &QPushButton::clicked, this, &MainWindow::onFileUrlBtnClicked);
    connect(m_startTransBtn, &QPushButton::clicked, this, &MainWindow::onStartTransBtnClicked);

    // 创建线程和工作对象
    m_workerThread = new QThread(this);
    m_sftpThread = new SftpThread();
    m_sftpThread->moveToThread(m_workerThread);
    // 连接工作对象的信号
    connect(m_sftpThread, &SftpThread::progress, this, &MainWindow::onProgress);
    connect(m_sftpThread, &SftpThread::finished, this, &MainWindow::onUploadFinished);

    // 启动线程
    m_workerThread->start();
}

MainWindow::~MainWindow() {
    m_workerThread->quit();
    m_workerThread->wait();
    delete m_sftpThread;
}

void MainWindow::onFileUrlBtnClicked() {
    // 创建文件对话框
    const QString filePath = QFileDialog::getOpenFileName(this,
        "选择文件",
        QDir::homePath(),
        "所有文件 (*.*);;文本文件 (*.txt);;图像文件 (*.png *.jpg *.bmp);;PDF文件 (*.pdf)");

    // 检查用户是否选择了文件
    if (filePath.isEmpty()) {
        m_statusBar->showMessage("已取消选择文件", 3000);
        return;
    }
    // 更新UI
    m_fileUrlLineEdit->setText(filePath);
}

void MainWindow::onStartTransBtnClicked(){
    // 检查是否选择了文件
    if (m_fileUrlLineEdit->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择要上传的文件");
        return;
    }
    // 显示进度条
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_progressLabel->setVisible(true);
    // 准备参数
    const QString fileName = m_fileUrlLineEdit->text().split("/").last();
    const sRemoteDeviceInfo deviceInfo("192.168.80.128",22,"rpdzkj"," ");
    const sTransFilePath tranPath(m_fileUrlLineEdit->text(),"/home/rpdzkj/" + fileName);

    // 禁用按钮防止重复点击
    m_startTransBtn->setEnabled(false);
    m_fileUrlBtn->setEnabled(false);
    // 使用工作线程中的工作对象执行上传
    QMetaObject::invokeMethod(m_sftpThread, "uploadFile",
        Q_ARG(sRemoteDeviceInfo, deviceInfo),
        Q_ARG(sTransFilePath, tranPath));
}

void MainWindow::onUploadFinished(const bool success, const QString &error) {
    // 重新启用按钮
    m_startTransBtn->setEnabled(true);
    m_fileUrlBtn->setEnabled(true);

    if (success) {
        QMessageBox::information(this, "成功", "文件上传成功");
    } else {
        QMessageBox::critical(this, "错误", "上传失败: " + error);
    }
}

void MainWindow::onProgress(const qint64 current, const qint64 total) const {
    // 避免除零错误
    if (total <= 0) return;

    // 计算百分比
    const int percent = static_cast<int>((current * 100) / total);

    // 避免不必要的更新
    if (percent != m_progressBar->value()) {
        m_progressBar->setValue(percent);
    }

    // 更新状态栏信息
    static int lastPercent = -1;
    if (percent != lastPercent && percent % 10 == 0) {
        const QString status = QString("已传输: %1/%2 MB")
                         .arg(static_cast<double>(current) / (1024.0 * 1024.0), 0, 'f', 1)
                         .arg(static_cast<double>(total) / (1024.0 * 1024.0), 0, 'f', 1);
        m_statusBar->showMessage(status);
        lastPercent = percent;
    }
}
