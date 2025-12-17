/**
 * @brief   :文件传输接口.
 * @author  :LiJunJie
 * @date    :2025/12/16
 * */

#ifndef SFTP_CLIENT_H
#define SFTP_CLIENT_H
///头文件声明
#include <QObject>
#include <libssh/libssh.h>
#include <QFile>
#include <QDebug>
#include <utility>
///结构体枚举声明
struct sRemoteDeviceInfo {
    QString host;           //远程设备ip
    int port{};               //远程设备端口
    QString username;       //远程设备用户名
    QString password;       //远程设备密码
    sRemoteDeviceInfo() = default;
    sRemoteDeviceInfo(QString  host, const int port, QString  username, QString  password)
        : host(std::move(host)), port(port), username(std::move(username)), password(std::move(password)) {}
};
struct sTrasFilePath {
    QString hostPath;       //本地端文件地址
    QString remotePath;     //远程设备文件地址
    sTrasFilePath() = default;
    sTrasFilePath(QString  hostPath, QString  remotePath)
        : hostPath(std::move(hostPath)), remotePath(std::move(remotePath)) {}
};

class SftpClient final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief       :构造.
     * @param [in]  :parent(default=nullptr)
     * */
    explicit SftpClient(QObject *parent = nullptr);

    /**
     * @brief       :析构.
     * */
    ~SftpClient() override;

    /**
     * @brief       :上传文件到远端设备.
     * @param [in]  :info(远程设备信息)
     * @param [in]  :path(传输文件路径)
     * */
    void UploadFile(const sRemoteDeviceInfo &info, const sTrasFilePath &path);

    /**
     * @brief       :从远端设备下载文件.
     * @param [in]  :info(远程设备信息)
     * @param [in]  :path(传输文件路径)
     * */
    void DownloadFile(const sRemoteDeviceInfo &info, const sTrasFilePath &path);

    /**
     * @brief       :在远端设备执行命令.
     * @param [in]  :info(远程设备信息)
     * @param [in]  :cmd(执行命令)
     * */
    void ExecuteCommand(const sRemoteDeviceInfo &info, const QString &cmd);

signals:
    /**
     * @brief       :获取传输进度.
     * @param [in]  :sent(已传输的字节数)
     * @param [in]  :total(需传输的总字节数)
     * */
    void progress(qint64 sent, qint64 total);
    /**
     * @brief       :传输终止标志.
     * @param [in]  :success(停止标志)
     * @param [in]  :error(完成/错误信息)
     * */
    void finished(bool success, const QString &error);
    /**
     * @brief       :执行命令回读.
     * @param [in]  :success(停止标志)
     * @param [in]  :error(完成/错误信息)
     * */
    void commandExecuted(const QString &output);

private:
    ssh_session m_session;      //远程连接句柄
    QFile m_localFile;          //读写文件临时句柄

    /**
     * @brief       :连接到远程设备.
     * @param [in]  :info(远程设备信息)
     * */
    bool ConnectToServer(const sRemoteDeviceInfo &info);

    /**
     * @brief       :清理资源并回收.
     * */
    void CleanupSession();
};

Q_DECLARE_METATYPE(sRemoteDeviceInfo)
Q_DECLARE_METATYPE(sTrasFilePath)
#endif // SFTP_CLIENT_H
