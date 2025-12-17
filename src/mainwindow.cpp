#include "mainwindow.h"
#include <QFileDialog>
#include <QStatusBar>

MainWindow::MainWindow(QMainWindow *parent) : QMainWindow(parent) {
    setWindowTitle("远程更新");
    this->resize(800, 600);
    m_statusBar = new QStatusBar(this);
    setStatusBar(m_statusBar);
    mainWidget = new QWidget;
    this->setCentralWidget(mainWidget);
    m_fileUrlButton = new QPushButton("打开");
    m_fileUrlButton->setMaximumSize(300,200);
    m_fileUrlLabel = new QLabel("文件路径：");
    m_fileUrlLabel->setWordWrap(true);
    m_fileUrlLineEdit = new QLineEdit();
    m_fileUrlLineEdit->setReadOnly(true);
    m_fileUrlLineEdit->setPlaceholderText("文件路径将显示在这里");
    m_progressBar = new QProgressBar;
    mainLayout = new QGridLayout;
    mainLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
    mainLayout->setSpacing(50);
    mainLayout->setContentsMargins(5,5,5,5);
    mainLayout->addWidget(m_fileUrlLabel, 0, 0, 1, 1);
    mainLayout->addWidget(m_fileUrlLineEdit, 0, 1, 1, 4);
    mainLayout->addWidget(m_fileUrlButton, 1, 0, 1, 1);
    mainLayout->addWidget(m_progressBar, 1, 1, 1, 4);
    mainWidget->setLayout(mainLayout);

    connect(m_fileUrlButton, &QPushButton::clicked, this, &MainWindow::on_fileUrlBtn_clicked);
}

MainWindow::~MainWindow() = default;

void MainWindow::on_fileUrlBtn_clicked() {
    // 创建文件对话框
    QString filePath = QFileDialog::getOpenFileName(this,
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