#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ispiocontroller.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionIoController_triggered();

private:
    Ui::MainWindow *ui;
    IspIoController *ispiocontroller = NULL;
};

#endif // MAINWINDOW_H
