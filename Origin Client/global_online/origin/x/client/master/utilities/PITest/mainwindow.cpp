#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "AppController.h"

MainWindow::MainWindow(AppController* controller, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _controller(controller)
{
    ui->setupUi(this);

    QObject::connect(_controller, SIGNAL(LogMessage(QString)), this, SLOT(onLogMessage(QString)), Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onLogMessage(QString msg)
{
    ui->txtMainLog->append(msg);
}
