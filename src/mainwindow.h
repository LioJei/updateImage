/**
 * @brief   :程序主窗口显示.
 * @author  :LiJunJie
 * @date    :2025/12/16
 */

#ifndef __TEST_MAIN_WINDOW_H
#define __TEST_MAIN_WINDOW_H
///头文件声明
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QMainWindow>
#include <QLabel>
#include <QProgressBar>
#include <QWidget>
#include "sftpclient.h"

class MainWindow : public QMainWindow {
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
    QWidget *mainWidget;                    //主窗口
    QGridLayout *mainLayout;                //主窗口布局
    QLabel *m_fileUrlLabel;                 //文件路径标签
    QLineEdit *m_fileUrlLineEdit;           //显示文件路径
    QPushButton *m_fileUrlButton;           //打开文件按钮
    QStatusBar *m_statusBar;                //状态栏
    QProgressBar *m_progressBar;            //传输进度条

private slots:

    ///打开文件响应槽函数
    void on_fileUrlBtn_clicked();

};

#endif //__TEST_MAIN_WINDOW_H
