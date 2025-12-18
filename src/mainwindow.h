/**
 * @brief   :程序主窗口显示.
 * @author  :LiJunJie
 * @date    :2025/12/16
 */

#ifndef TEST_MAIN_WINDOW_H
#define TEST_MAIN_WINDOW_H
///头文件声明
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QMainWindow>
#include <QLabel>
#include <QProgressBar>
#include <QWidget>
#include "sftpthread.h"

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief       :构造.
     * */
    explicit MainWindow(QMainWindow *parent = nullptr);

    /**
     * @brief       :析构.
     * */
    ~MainWindow() override;

private slots:
    /**
     * @brief       :文件打开响应槽函数.
     * */
    void onFileUrlBtnClicked();

    /**
     * @brief       :文件传输响应槽函数.
     * */
    void onStartTransBtnClicked();

    /**
     * @brief       :IP配置响应槽函数.
     * */
    void onIpBtnClicked();

    /**
     * @brief       :传输终止响应槽函数.
     * @param [in]  :success(成功/失败)
     * @param [in]  :error(打印信息)
     * */
    void onUploadFinished(bool success, const QString &error) const;

    /**
     * @brief       :执行命令回读.
     * @param [in]  :success(停止标志)
     * @param [in]  :error(完成/错误信息)
     * */
    void onCommandExecuted(const QString &output, const QString &error);

    /**
     * @brief       :传输进度响应槽函数.
     * @param [in]  :current(已传输的字节数)
     * @param [in]  :total(需传输的总字节数)
     * */
    void onProgress(qint64 current, qint64 total);

    /**
     * @brief       :获取单个文件传输进度.
     * @param [in]  :fileIndex(文件索引)
     * @param [in]  :fileName(文件名)
     * @param [in]  :current(已传输的字节数)
     * @param [in]  :total(需传输的总字节数)
     * */
    void onFileProgress(int fileIndex, const QString &fileName, qint64 current, qint64 total);

private:
    /**
     * @brief       :界面初始化
     * */
    void setupUI();

    /**
     * @brief       :文件进度条更新
     * @param [in]  :current(已传输的字节数)
     * @param [in]  :total(需传输的总字节数)
     * */
    void updateProgressDisplay(qint64 current, qint64 total);

    /**
      * @brief       :文件标签更新
      * @param [in]  :fileIndex(文件索引)
      * @param [in]  :fileName(文件名)
      * @param [in]  :current(已传输的字节数)
      * @param [in]  :total(需传输的总字节数)
      * */
    void updateFileProgress(int fileIndex, const QString &fileName, qint64 current, qint64 total);

    QWidget *mainWidget{}; //主窗口
    QGridLayout *mainLayout{}; //主窗口布局
    QStatusBar *m_statusBar{}; //状态栏
    //第1行显示[IP配置]
    QLabel *m_ipLabel{}; //ip标签
    QLabel *m_curIpLabel{}; //当前ip显示标签
    QLineEdit *m_ipLineEdit{}; //ip文本栏
    QPushButton *m_ipBtn{}; //ip确认按钮
    //第2行显示[文件操作]
    QLabel *m_fileUrlLabel{}; //文件路径标签
    QLineEdit *m_fileUrlLineEdit{}; //显示文件路径
    QPushButton *m_fileUrlBtn{}; //打开文件按钮
    //第3行显示[进度条]
    QLabel *m_progressLabel{}; //进度标签
    QProgressBar *m_progressBar{}; //传输进度条
    QPushButton *m_startTransBtn{}; //开始传输按钮
    //[线程工作对象]
    SftpThread *m_sftpThread; //sftp线程句柄
    QThread *m_workerThread; //任务线程句柄
    //[数据]
    QString m_ip; //ip
    QVector<sTransFilePath> m_filePaths; //传输文件路径容器
    qint64 m_totalBytes = 0; //总文件大小
    qint64 m_totalTransferredBytes = 0; //已传输大小
    int m_currentFileIndex = 0; //当前文件索引
    int m_totalFiles = 0; //总传输文件数
};

#endif //TEST_MAIN_WINDOW_H
