#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow wim;
    wim.show();

    // SftpClient ssh;
    //
    // // 连接进度信号
    // QObject::connect(&ssh, &SftpClient::progress,
    //                 [](qint64 current, qint64 total) {
    //     qDebug() << "进度:" << current * 100 / total << "%";
    // });
    //
    // // 连接完成信号
    // QObject::connect(&ssh, &SftpClient::finished,
    //                 [&](bool success, const QString &error) {
    //     if (success) qDebug() << "操作成功！";
    //     else qCritical() << "错误:" << error;
    //     QApplication::quit();
    // });
    //
    // // 选择上传或下载
    // bool upload = false; // 设置为 false 进行下载
    //
    // if (upload) {
    //     // 上传文件
    //     ssh.uploadFile("192.168.80.128", 22,
    //                   "rpdzkj", " ",
    //                   "../uptest.txt",
    //                   "/home/rpdzkj/setup/up.txt");
    // } else {
    //     // 下载文件
    //     ssh.downloadFile("192.168.80.128", 22,
    //                   "rpdzkj", " ",
    //                     "/home/rpdzkj/setup/downtest.txt",
    //                     "../download.txt");
    // }
    
    return a.exec();
}
