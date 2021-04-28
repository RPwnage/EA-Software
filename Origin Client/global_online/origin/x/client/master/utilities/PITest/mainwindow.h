#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class AppController;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(AppController* controller, QWidget *parent = 0);
    ~MainWindow();

private slots:
    void onLogMessage(QString msg);

private:
    Ui::MainWindow *ui;
    AppController* _controller;
};

#endif // MAINWINDOW_H
