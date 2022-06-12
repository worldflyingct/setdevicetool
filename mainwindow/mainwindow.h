#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ispiocontroller/ispiocontroller.h"
#include "isp485/isp485.h"
#include "ispcollector/ispcollector.h"
#include "ispiotprogram/ispiotprogram.h"
#include "stmisp/stmisp.h"
#include "uartassist/uartassist.h"
#include "netassist/netassist.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionAboutQt_triggered();
    void on_actionAbout_triggered();
    void on_actionIspIoController_triggered();
    void on_actionIsp485GateWay_triggered();
    void on_actionIspCollector_triggered();
    void on_actionIspIotProgram_triggered();
    void on_actionStmIsp_triggered();
    void on_actionUartAssist_triggered();
    void on_actionNetAssist_triggered();

private:
    Ui::MainWindow *ui;
    int FreeOthersWidget(void *widget);
    IspIoController *ispiocontroller = NULL;
    Isp485 *isp485 = NULL;
    IspCollector *ispcollector = NULL;
    IspIotProgram *ispiotprogram = NULL;
    StmIsp *stmisp = NULL;
    UartAssist *uartassist = NULL;
    NetAssist *netassist = NULL;
};

#endif // MAINWINDOW_H
