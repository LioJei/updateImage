#include "sftpthread.h"

#include <QThread>

SftpThread::SftpThread(QObject *parent) : QObject(parent) {
    // 连接SftpClient的信号到SftpWorker的信号，以便转发到主线程
    connect(&m_sftpClient, &SftpClient::progress, this, &SftpThread::progress);
    connect(&m_sftpClient, &SftpClient::fileProgress, this, &SftpThread::fileProgress);
    connect(&m_sftpClient, &SftpClient::commandExecuted, this, &SftpThread::commandExecuted);
    // 连接SftpClient的信号到SftpThread的槽函数，用于处理单个文件的完成事件
    connect(&m_sftpClient, &SftpClient::finished, this, &SftpThread::onFileUploadFinished);
}

// 新增的槽函数
void SftpThread::onFileUploadFinished(bool success, const QString &error)
{
    qDebug() << "[SftpThread] File upload finished. Success:" << success << "Error:" << error;
    qDebug() << "[SftpThread] Current file index:" << m_currentFileIndex << "of" << m_totalFiles;

    // 如果上传失败，立即终止整个流程并发射 finished 信号
    if (!success) {
        emit finished(false, error);
        return;
    }

    // 当前文件上传成功，继续处理下一个
    m_currentFileIndex++;

    if (m_currentFileIndex < m_totalFiles) {
        qDebug() << "[SftpThread] Starting next file:" << m_currentPaths[m_currentFileIndex].fileName;
        uploadNextFile(); // 上传下一个文件
    } else {
        // 所有文件都已成功上传
        qDebug() << "[SftpThread] All files uploaded successfully!";
        emit finished(true, "所有文件上传成功");
    }
}


void SftpThread::uploadNextFile()
{
    // 由 onFileUploadFinished 调用
    const sTransFilePath &currentPath = m_currentPaths[m_currentFileIndex];
    m_currentFileName = currentPath.fileName;

    qDebug() << "[SftpThread] Uploading file:" << m_currentFileName << "at index" << m_currentFileIndex;

    // 调用SftpClient上传单个文件
    m_sftpClient.UploadFile(m_currentDeviceInfo, currentPath);
}

void SftpThread::uploadFile(const sRemoteDeviceInfo &info, const QVector<sTransFilePath> &paths) {
    m_currentDeviceInfo = info;
    m_currentPaths = paths;
    m_totalFiles = paths.size();
    m_currentFileIndex = 0;

    // 如果文件列表为空，直接结束
    if (m_currentPaths.isEmpty()) {
        emit finished(true, "没有文件需要上传");
        return;
    }

    qDebug() << "[SftpThread] Starting upload of" << m_totalFiles << "files.";

    // 开始上传第一个文件
    uploadNextFile();
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
