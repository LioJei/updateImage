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

private:
    QWidget *mainWidget;                //主窗口
    QGridLayout *mainLayout;            //主窗口布局
    QLabel *m_fileUrlLabel;             //文件路径标签
    QLineEdit *m_fileUrlLineEdit;       //显示文件路径
    QPushButton *m_fileUrlBtn;          //打开文件按钮
    QStatusBar *m_statusBar;            //状态栏
    QLabel *m_progressLabel;            //进度标签
    QProgressBar *m_progressBar;        //传输进度条
    QPushButton *m_startTransBtn;       //开始传输按钮

    SftpThread *m_sftpThread;           //sftp线程句柄
    QThread *m_workerThread;            //任务线程句柄

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
     * @brief       :传输终止响应槽函数.
     * @param [in]  :success(成功/失败)
     * @param [in]  :error(打印信息)
     * */
    void onUploadFinished(bool success, const QString &error);

    /**
     * @brief       :传输进度响应槽函数.
     * @param [in]  :current(已传输的字节数)
     * @param [in]  :total(需传输的总字节数)
     * */
    void onProgress(qint64 current, qint64 total) const;
};

#endif //TEST_MAIN_WINDOW_H
