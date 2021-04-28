#include "mainwindow.h"
#include "AppController.h"
#include <QApplication>
#include <QThread>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Leak these, who cares
    QThread *controllerThread = new QThread();
    controllerThread->start();
    AppController *appController = new AppController();
    appController->moveToThread(controllerThread);

    MainWindow w(appController);
    w.show();

    // Run the app startup
    QMetaObject::invokeMethod(appController, "onStartup", Qt::QueuedConnection);

    return a.exec();
}
