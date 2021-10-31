#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ispiocontroller/ispiocontroller.h"
#include "isp485/isp485.h"

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
    void on_action485GateWay_triggered();
    void on_actionAbout_triggered();

private:
    Ui::MainWindow *ui;
    void FreeOthersWidget(void *widget);
    IspIoController *ispiocontroller = NULL;
    Isp485 *isp485 = NULL;
};

#endif // MAINWINDOW_H