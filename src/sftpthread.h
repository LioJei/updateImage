/**
* @brief   :SFTP中间线程透传接口.
 * @author  :LiJunJie
 * @date    :2025/12/16
 * */
#ifndef SFTP_THREAD_H
#define SFTP_THREAD_H

#include <QObject>
#include "sftpclient.h"

class SftpThread final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief       :构造.
     * @param [in]  :parent(default=nullptr)
     * */
    explicit SftpThread(QObject *parent = nullptr);

public slots:
    /**
     * @brief       :上传文件到远端设备透传接口.
     * @param [in]  :info(远程设备信息)
     * @param [in]  :path(传输文件路径)
     * */
    void uploadFile(const sRemoteDeviceInfo &info, const sTrasFilePath &path);

    /**
     * @brief       :从远端设备下载文件透传接口.
     * @param [in]  :info(远程设备信息)
     * @param [in]  :path(传输文件路径)
     * */
    void downloadFile(const sRemoteDeviceInfo &info, const sTrasFilePath &path);

    /**
     * @brief       :在远端设备执行命令透传接口.
     * @param [in]  :info(远程设备信息)
     * @param [in]  :cmd(执行命令)
     * */
    void executeCommand(const sRemoteDeviceInfo &info, const QString &cmd);

signals:
    /**
     * @brief       :获取传输进度透传接口.
     * @param [in]  :sent(已传输的字节数)
     * @param [in]  :total(需传输的总字节数)
     * */
    void progress(qint64 current, qint64 total);

    /**
     * @brief       :传输终止标志透传接口.
     * @param [in]  :success(停止标志)
     * @param [in]  :error(完成/错误信息)
     * */
    void finished(bool success, const QString &error);

    /**
     * @brief       :执行命令回读透传接口.
     * @param [in]  :success(停止标志)
     * @param [in]  :error(完成/错误信息)
     * */
    void commandExecuted(const QString &output);

private:
    SftpClient m_sftpClient; //实际SFTP实例
};

#endif // SFTP_THREAD_H
