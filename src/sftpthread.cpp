#include "sftpthread.h"

#include <QThread>

SftpThread::SftpThread(QObject *parent) : QObject(parent) {
    // 连接SftpClient的信号到SftpWorker的信号，以便转发到主线程
    connect(&m_sftpClient, &SftpClient::progress, this, &SftpThread::progress);
    connect(&m_sftpClient, &SftpClient::finished, this, &SftpThread::finished);
    connect(&m_sftpClient, &SftpClient::commandExecuted, this, &SftpThread::commandExecuted);
}

void SftpThread::uploadFile(const sRemoteDeviceInfo &info, const sTransFilePath &path) {
    m_sftpClient.UploadFile(info, path);
}

void SftpThread::downloadFile(const sRemoteDeviceInfo &info, const sTransFilePath &path) {
    m_sftpClient.DownloadFile(info, path);
}

void SftpThread::executeCommand(const sRemoteDeviceInfo &info, const QString &cmd) {
    m_sftpClient.ExecuteCommand(info, cmd);
}

void SftpThread::cancelOperation() {
    m_sftpClient.cancelOperation();
}
