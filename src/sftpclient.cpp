#include "sftpclient.h"
#include <QThread>
#include <QDebug>
#include <fcntl.h>

// Windows 平台兼容定义
#ifdef _WIN32
#include <io.h>
#ifndef O_WRONLY
#define O_WRONLY _O_WRONLY
#endif
#ifndef O_CREAT
#define O_CREAT _O_CREAT
#endif
#ifndef O_TRUNC
#define O_TRUNC _O_TRUNC
#endif
#ifndef O_RDONLY
#define O_RDONLY _O_RDONLY
#endif
#else
#include <unistd.h>
#endif

constexpr int SSH_TIMEOUT = 30000; // 30秒
constexpr int BUFFER_SIZE = 10 * 1024 * 1024; // 1MB 块
constexpr int DOWN_BUFFER_SIZE = 16384; // 16KB 块
constexpr int MAX_WRITE_SIZE = 32 * 1024; // 32KB 每次写入的最大值
constexpr qint64 MIN_PROGRESS_INTERVAL = 100 * 1024 * 1024; // 每 50MB 更新一次

SftpClient::SftpClient(QObject *parent) : QObject(parent), m_session(nullptr) {
    //调用ssh库初始化接口
    ssh_init();
    // 设置操作计时器
    connect(&m_operationTimer, &QTimer::timeout, [this]() {
        if (m_sftp) {
            // 定期检查取消请求
            if (m_cancelRequested.load()) {
                qDebug() << "Operation canceled during transfer";
            }
        }
    });
    m_operationTimer.start(1000); // 每秒检查一次
}

SftpClient::~SftpClient() {
    CleanupSession();
    //资源清理回收完成后调用库释放接口
    ssh_finalize();
}

void SftpClient::CleanupSession() {
    if (m_sftp) {
        sftp_free(m_sftp);
        m_sftp = nullptr;
    }
    if (m_session) {
        ssh_disconnect(m_session);
        ssh_free(m_session);
        m_session = nullptr;
    }
}

bool SftpClient::ConnectToServer(const sRemoteDeviceInfo &info) {
    CleanupSession(); // 清理之前的会话

    // 创建 SSH 会话
    m_session = ssh_new();
    if (!m_session) {
        emit finished(false, "创建SSH会话失败");
        return false;
    }

    // 配置连接参数
    ssh_options_set(m_session, SSH_OPTIONS_HOST, info.host.toUtf8().constData());
    ssh_options_set(m_session, SSH_OPTIONS_PORT, &info.port);
    ssh_options_set(m_session, SSH_OPTIONS_USER, info.username.toUtf8().constData());

    // 设置超时
    ssh_options_set(m_session, SSH_OPTIONS_TIMEOUT, &SSH_TIMEOUT);


    // 连接服务器
    if (ssh_connect(m_session) != SSH_OK) {
        emit finished(false, "连接服务器失败: " + QString(ssh_get_error(m_session)));
        ssh_free(m_session);
        m_session = nullptr;
        return false;
    }

    // 身份认证
    if (ssh_userauth_password(m_session, nullptr, info.password.toUtf8().constData()) != SSH_AUTH_SUCCESS) {
        emit finished(false, "认证失败: " + QString(ssh_get_error(m_session)));
        ssh_disconnect(m_session);
        ssh_free(m_session);
        m_session = nullptr;
        return false;
    }

    // 创建 SFTP 会话
    m_sftp = sftp_new(m_session);
    if (!m_sftp) {
        emit finished(false, "创建SFTP会话失败");
        ssh_disconnect(m_session);
        ssh_free(m_session);
        m_session = nullptr;
        return false;
    }

    if (sftp_init(m_sftp) != SSH_OK) {
        emit finished(false, "初始化SFTP失败");
        sftp_free(m_sftp);
        m_sftp = nullptr;
        ssh_disconnect(m_session);
        ssh_free(m_session);
        m_session = nullptr;
        return false;
    }

    return true;
}

void SftpClient::UploadFile(const sRemoteDeviceInfo &info, const sTransFilePath &path) {
    m_cancelRequested.store(false);
    QElapsedTimer operationTimer;
    operationTimer.start();
    // 1. 打开本地文件
    QFile m_localFile(path.hostPath);
    if (!m_localFile.open(QIODevice::ReadOnly)) {
        emit finished(false, "打开本地文件失败" + m_localFile.errorString());
        emit fileProgress(-1, path.fileName, 0, path.fileSize);
        return;
    }

    // 2. 连接服务器
    if (!ConnectToServer(info)) {
        m_localFile.close();
        return;
    }

    // 3. 打开远程文件
    sftp_file remote_file = sftp_open(m_sftp, path.remotePath.toUtf8().constData(),
                                      O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (!remote_file) {
        CleanupSession();
        m_localFile.close();
        emit finished(false, "创建远程文件失败" + QString(sftp_get_error(m_sftp)));
        emit fileProgress(-1, path.fileName, 0, path.fileSize);
        return;
    }

    // 4. 分块上传文件
    const QScopedArrayPointer<char> buffer(new char[BUFFER_SIZE]);
    const qint64 totalSize = m_localFile.size();
    qint64 bytesSent = 0;
    qint64 lastEmittedBytes = 0;
    // 发送单个文件开始信号
    emit fileProgress(0, path.fileName, 0, totalSize);

    while (!m_localFile.atEnd() && !m_cancelRequested.load()) {
        // 检查取消请求
        if (m_cancelRequested.load()) {
            qDebug() << "Upload canceled for file: " << path.fileName;
            break;
        }
        // 读取数据块
        const qint64 bytesRead = m_localFile.read(buffer.data(), BUFFER_SIZE);
        if (bytesRead <= 0) break;

        // 写入远程文件
        // 将读取的数据分成小块写入
        qint64 bytesWrittenInBlock = 0;
        while (bytesWrittenInBlock < bytesRead) {
            // 计算本次写入的大小（不超过MAX_WRITE_SIZE）
            const qint64 chunkSize = qMin(static_cast<qint64>(MAX_WRITE_SIZE), bytesRead - bytesWrittenInBlock);

            // 写入远程文件
            const ssize_t bytesWritten = sftp_write(remote_file,
                                                    buffer.data() + bytesWrittenInBlock,
                                                    static_cast<uint32_t>(chunkSize));

            if (bytesWritten != chunkSize) {
                qDebug() << QString("Write failed: %1 (try to write: %2, Actual Write: %3)")
                        .arg(sftp_get_error(m_sftp))
                        .arg(chunkSize)
                        .arg(bytesWritten);
                sftp_close(remote_file);
                CleanupSession();
                m_localFile.close();
                emit finished(false, "写入失败:" + QString(sftp_get_error(m_sftp)));
                emit fileProgress(-1, path.fileName, bytesSent, totalSize);
                return;
            }

            bytesWrittenInBlock += bytesWritten;
            bytesSent += bytesWritten;

            // 控制信号发射频率
            if (bytesSent - lastEmittedBytes >= MIN_PROGRESS_INTERVAL ||
                bytesSent == totalSize ||
                operationTimer.elapsed() > 5000) {
                // 发送单个文件进度
                emit fileProgress(0, path.fileName, bytesSent, totalSize);
                qDebug() << "UpProgress:" << bytesSent << "/" << totalSize
                        << "(" << (bytesSent * 100 / totalSize) << "%)";
                emit progress(bytesSent, totalSize);
                lastEmittedBytes = bytesSent;
                operationTimer.restart();
            }
        }
    }

    // 5. 清理资源
    sftp_close(remote_file);
    CleanupSession();
    m_localFile.close();
    if (m_cancelRequested.load()) {
        qDebug() << "upload canceled for file:" << path.fileName;
        emit finished(false, "操作已取消: " + path.fileName);
        emit fileProgress(-1, path.fileName, bytesSent, totalSize);
    } else if (bytesSent == totalSize) {
        qDebug() << "upload finished for file:" << path.fileName;
        emit finished(true, "上传成功: " + path.fileName);
        emit fileProgress(1, path.fileName, totalSize, totalSize);
    } else {
        qDebug() << QString("upload failed for %1: %2/%3 bytes")
                .arg(path.fileName)
                .arg(bytesSent)
                .arg(totalSize);
        emit finished(false, QString("上传未完成: %1 %2/%3 字节")
                      .arg(path.fileName)
                      .arg(bytesSent)
                      .arg(totalSize));
        emit fileProgress(-1, path.fileName, bytesSent, totalSize);
    }
}

void SftpClient::DownloadFile(const sRemoteDeviceInfo &info, const sTransFilePath &path) {
    // 1. 连接服务器
    if (!ConnectToServer(info)) {
        return;
    }

    // 2. 打开远程文件
    sftp_file remote_file = sftp_open(m_sftp, path.remotePath.toUtf8().constData(), O_RDONLY, 0);
    if (!remote_file) {
        CleanupSession();
        emit finished(false, "打开远程文件失败: " + QString(sftp_get_error(m_sftp)));
        return;
    }

    // 3. 获取文件大小
    sftp_attributes file_attr = sftp_fstat(remote_file);
    if (!file_attr) {
        sftp_close(remote_file);
        CleanupSession();
        emit finished(false, "获取文件大小失败");
        return;
    }
    const auto totalSize = static_cast<qint64>(file_attr->size);
    sftp_attributes_free(file_attr);

    // 4. 创建本地文件
    QFile localFile(path.hostPath);
    if (!localFile.open(QIODevice::WriteOnly)) {
        sftp_close(remote_file);
        CleanupSession();
        emit finished(false, "创建本地文件失败: " + localFile.errorString());
        return;
    }

    // 5. 分块下载文件
    char *buffer = new char[DOWN_BUFFER_SIZE];
    qint64 bytesReceived = 0;

    while (true) {
        if (m_cancelRequested.load()) break;
        const ssize_t bytesRead = sftp_read(remote_file, buffer, DOWN_BUFFER_SIZE);
        if (bytesRead == 0) break; // 文件结束
        if (bytesRead < 0) {
            sftp_close(remote_file);
            CleanupSession();
            localFile.close();
            delete [] buffer;
            emit finished(false, "读取远程文件失败: " + QString(sftp_get_error(m_sftp)));
            return;
        }

        // 写入本地文件
        const qint64 bytesWritten = localFile.write(buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            sftp_close(remote_file);
            CleanupSession();
            localFile.close();
            delete[] buffer;
            emit finished(false, "写入本地文件失败: " + localFile.errorString());
            return;
        }

        bytesReceived += bytesWritten;
        emit progress(bytesReceived, totalSize); // 发送进度信号
        QThread::msleep(10); // 避免 CPU 占用过高
    }

    // 6. 清理资源
    delete[] buffer;
    sftp_close(remote_file);
    CleanupSession();
    localFile.close();

    if (m_cancelRequested.load()) {
        emit finished(false, "下载已取消");
    } else {
        emit finished(true, "下载成功");
    }
}

void SftpClient::ExecuteCommand(const sRemoteDeviceInfo &info, const QString &cmd) {
    //连接服务器
    if (!ConnectToServer(info)) {
        return;
    }

    //创建命令通道
    ssh_channel channel = ssh_channel_new(m_session);
    if (!channel) {
        emit finished(false, "创建命令通道失败");
        return;
    }

    //打开通道
    if (ssh_channel_open_session(channel) != SSH_OK) {
        ssh_channel_free(channel);
        emit finished(false, "打开通道失败");
        return;
    }

    // 执行命令
    if (ssh_channel_request_exec(channel, cmd.toUtf8().constData()) != SSH_OK) {
        qDebug() << "执行命令失败";
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        emit finished(false, "执行命令失败");
        return;
    }

    // 读取输出
    char buffer[1024];
    int nBytes;
    QString output;

    while ((nBytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0))) {
        if (nBytes < 0) {
            qDebug() << "读取输出错误";
            break;
        }
        qDebug() << buffer;
        output.append(QByteArray(buffer, nBytes));
    }

    // 读取错误输出
    QString errorOutput;
    while ((nBytes = ssh_channel_read(channel, buffer, sizeof(buffer), 1))) {
        if (nBytes < 0) {
            qDebug() << "读取错误输出错误";
            break;
        }
        errorOutput.append(QByteArray(buffer, nBytes));
    }

    // 获取退出状态
    if (ssh_channel_send_eof(channel) != SSH_OK) {
        qDebug() << "发送EOF失败";
    }

    // 获取退出状态和信号信息（使用新函数）
    uint32_t exit_code = 0;
    char *exit_signal = nullptr;
    int core_dumped = 0;

    int rc = ssh_channel_get_exit_state(channel, &exit_code, &exit_signal, &core_dumped);
    if (rc == SSH_OK) {
        // 如果有退出信号，可以记录或处理
        if (exit_signal) {
            qDebug() << "命令被信号终止:" << exit_signal << (core_dumped ? " (core dumped)" : "");
            // 释放信号字符串
            ssh_string_free_char(exit_signal);
        }
    }

    // 关闭通道
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    emit commandExecuted(output, errorOutput);
}

void SftpClient::cancelOperation() {
    m_cancelRequested.store(true);
}
