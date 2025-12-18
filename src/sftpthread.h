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

private:
    /**
    * @brief           :获取当前文件索引.
    * @return [int]    :m_currentFileIndex(当前文件索引)
    * */
    int getCurrentFileIndex() const { return m_currentFileIndex; }

    /**
     * @brief               :获取当前文件名.
     * @return [QString]    :m_currentFileName(当前文件名)
     * */
    QString getCurrentFileName() const { return m_currentFileName; }

    /**
     * @brief           :获取文件总数.
     * @return [int]    :m_totalFiles(文件总数)
     * */
    int getTotalFiles() const { return m_totalFiles; }

    /**
     * @brief           :递归上传文件.
     * */
    void uploadNextFile();

public slots:
    /**
     * @brief       :上传文件到远端设备透传接口.
     * @param [in]  :info(远程设备信息)
     * @param [in]  :path(传输文件路径)
     * */
    void uploadFile(const sRemoteDeviceInfo &info, const QVector<sTransFilePath> &paths);

    /**
     * @brief       :从远端设备下载文件透传接口.
     * @param [in]  :info(远程设备信息)
     * @param [in]  :path(传输文件路径)
     * */
    void downloadFile(const sRemoteDeviceInfo &info, const sTransFilePath &path);

    /**
     * @brief       :在远端设备执行命令透传接口.
     * @param [in]  :info(远程设备信息)
     * @param [in]  :cmd(执行命令)
     * */
    void executeCommand(const sRemoteDeviceInfo &info, const QString &cmd);

    /**
     * @brief       :取消文件传输操作.
     * */
    void cancelOperation();

private slots:
    /**
     * @brief       :单个文件传输终止标志接口.
     * @param [in]  :success(停止标志)
     * @param [in]  :error(完成/错误信息)
     * */
    void onFileUploadFinished(bool success, const QString &error);

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
    void commandExecuted(const QString &output, const QString &error);
    /**
     * @brief       :获取单个传输进度透传接口..
     * @param [in]  :fileIndex(文件索引)
     * @param [in]  :fileName(文件名)
     * @param [in]  :current(已传输的字节数)
     * @param [in]  :total(需传输的总字节数)
     * */
    void fileProgress(int fileIndex, const QString &fileName, qint64 current, qint64 total); // 单个文件进度

private:
    SftpClient m_sftpClient; //实际SFTP实例
    int m_currentFileIndex = 0; //当前文件索引
    QString m_currentFileName;  //当前文件名
    int m_totalFiles = 0;   //文件总数
    QVector<sTransFilePath> m_currentPaths; //文件路径容器
    sRemoteDeviceInfo m_currentDeviceInfo;  //远程设备信息
};

#endif // SFTP_THREAD_H
