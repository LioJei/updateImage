#include "sftpclient.h"
#include <QThread>
#include <QDebug>
#include <libssh/sftp.h>
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

SftpClient::SftpClient(QObject *parent) : QObject(parent), m_session(nullptr) {
    //调用ssh库初始化接口
    ssh_init();
}

SftpClient::~SftpClient() {
    CleanupSession();
    //资源清理回收完成后调用库释放接口
    ssh_finalize();
}

void SftpClient::CleanupSession() {
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

    // 连接服务器
    if (ssh_connect(m_session) != SSH_OK) {
        emit finished(false, "连接服务器失败: " + QString(ssh_get_error(m_session)));
        return false;
    }

    // 身份认证
    if (ssh_userauth_password(m_session, nullptr, info.password.toUtf8().constData()) != SSH_AUTH_SUCCESS) {
        emit finished(false, "认证失败: " + QString(ssh_get_error(m_session)));
        return false;
    }

    return true;
}

void SftpClient::UploadFile(const sRemoteDeviceInfo &info, const sTrasFilePath &path) {
    qDebug() << "开始上传文件:" << path.hostPath << "->" << path.remotePath;
    // 1. 打开本地文件
    m_localFile.setFileName(path.hostPath);
    if (!m_localFile.open(QIODevice::ReadOnly)) {
        emit finished(false, "打开本地文件失败");
        return;
    }

    // 2. 连接服务器
    if (!ConnectToServer(info)) {
        return;
    }

    // 3. 创建 SFTP 会话
    sftp_session sftp = sftp_new(m_session);
    if (!sftp) {
        emit finished(false, "创建SFTP会话失败");
        return;
    }
    if (sftp_init(sftp) != SSH_OK) {
        sftp_free(sftp);
        emit finished(false, "初始化SFTP失败");
        return;
    }

    // 4. 打开远程文件
    sftp_file remote_file = sftp_open(sftp, path.remotePath.toUtf8().constData(),
                                      O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (!remote_file) {
        sftp_free(sftp);
        emit finished(false, "创建远程文件失败");
        return;
    }

    // 5. 分块上传文件
    constexpr int BUFFER_SIZE = 16384; // 16KB 块
    char buffer[BUFFER_SIZE];
    qint64 totalSize = m_localFile.size();
    qint64 bytesSent = 0;

    while (!m_localFile.atEnd()) {
        qint64 bytesRead = m_localFile.read(buffer, BUFFER_SIZE);
        if (bytesRead <= 0) break;

        // 写入远程文件
        ssize_t byteWriten = sftp_write(remote_file, buffer, bytesRead);
        if (byteWriten != bytesRead) {
            sftp_close(remote_file);
            sftp_free(sftp);
            emit finished(false, "写入失败: " + QString(sftp_get_error(sftp)));
            return;
        }

        bytesSent += byteWriten;
        emit progress(bytesSent, totalSize); // 发送进度信号
        QThread::msleep(10); // 避免 CPU 占用过高
    }

    // 6. 清理资源
    sftp_close(remote_file);
    sftp_free(sftp);
    m_localFile.close();
    emit finished(true, "上传成功");
}

void SftpClient::DownloadFile(const sRemoteDeviceInfo &info, const sTrasFilePath &path) {
    // 1. 连接服务器
    if (!ConnectToServer(info)) {
        return;
    }

    // 2. 创建 SFTP 会话
    sftp_session sftp = sftp_new(m_session);
    if (!sftp) {
        emit finished(false, "创建SFTP会话失败");
        return;
    }
    if (sftp_init(sftp) != SSH_OK) {
        sftp_free(sftp);
        emit finished(false, "初始化SFTP失败");
        return;
    }

    // 3. 打开远程文件
    sftp_file remote_file = sftp_open(sftp, path.remotePath.toUtf8().constData(), O_RDONLY, 0);
    if (!remote_file) {
        sftp_free(sftp);
        emit finished(false, "打开远程文件失败");
        return;
    }

    // 4. 获取文件大小
    sftp_attributes file_attr = sftp_fstat(remote_file);
    if (!file_attr) {
        sftp_close(remote_file);
        sftp_free(sftp);
        emit finished(false, "获取文件大小失败");
        return;
    }
    auto totalSize = static_cast<qint64>(file_attr->size);
    sftp_attributes_free(file_attr);

    // 5. 创建本地文件
    QFile localFile(path.hostPath);
    if (!localFile.open(QIODevice::WriteOnly)) {
        sftp_close(remote_file);
        sftp_free(sftp);
        emit finished(false, "创建本地文件失败");
        return;
    }

    // 6. 分块下载文件
    constexpr int BUFFER_SIZE = 16384; // 16KB 块
    char buffer[BUFFER_SIZE];
    qint64 bytesReceived = 0;

    while (true) {
        ssize_t bytesRead = sftp_read(remote_file, buffer, BUFFER_SIZE);
        if (bytesRead == 0) break; // 文件结束
        if (bytesRead < 0) {
            sftp_close(remote_file);
            sftp_free(sftp);
            localFile.close();
            emit finished(false, "读取远程文件失败: " + QString(sftp_get_error(sftp)));
            return;
        }

        // 写入本地文件
        qint64 bytesWritten = localFile.write(buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            sftp_close(remote_file);
            sftp_free(sftp);
            localFile.close();
            emit finished(false, "写入本地文件失败");
            return;
        }

        bytesReceived += bytesWritten;
        emit progress(bytesReceived, totalSize); // 发送进度信号
        QThread::msleep(10); // 避免 CPU 占用过高
    }

    // 7. 清理资源
    sftp_close(remote_file);
    sftp_free(sftp);
    localFile.close();
    emit finished(true, "下载成功");
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
    int nbytes;
    QString output;

    while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0))) {
        if (nbytes < 0) {
            qDebug() << "读取输出错误";
            break;
        }
        qDebug() << buffer;
        output.append(QByteArray(buffer, nbytes));
    }

    // 读取错误输出
    QString errorOutput;
    while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 1))) {
        if (nbytes < 0) {
            qDebug() << "读取错误输出错误";
            break;
        }
        errorOutput.append(QByteArray(buffer, nbytes));
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
    } else {
        // 获取退出状态失败，设置一个非零值
        qDebug() << "获取退出状态失败";
    }

    // 关闭通道
    ssh_channel_close(channel);
    ssh_channel_free(channel);
}
