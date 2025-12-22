#include "mainwindow.h"
#include <json/json.h>
#include <shlwapi.h>
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // 注册自定义类型，以便用于跨线程的信号槽传递
    qRegisterMetaType<sRemoteDeviceInfo>("sRemoteDeviceInfo");
    qRegisterMetaType<sTransFilePath>("sTransFilePath");
    MainWindow wim;
    wim.show();
    
    return QApplication::exec();
}
