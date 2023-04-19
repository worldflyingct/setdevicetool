#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ispiocontroller/ispiocontroller.h"
#include "isp485/isp485.h"
#include "ispcollector/ispcollector.h"
#include "iotprogram/isp/ispiotprogram.h"
#include "iotprogram/rj45/rj45iotprogram.h"
#include "stmisp/stmisp.h"
#include "tkmisp/tkmisp.h"
#include "uartassist/uartassist.h"
#include "netassist/netassist.h"
#include "smartbuilding/22/smartbuilding22.h"
#include "tkm300config/tkm300config.h"

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
    void on_actionRj45IotProgram_triggered();
    void on_actionStmIsp_triggered();
    void on_actionUartAssist_triggered();
    void on_actionNetAssist_triggered();
    void on_actiontkm_triggered();
    void on_actionSmartbuilding22_triggered();
    void on_actionTkm300_triggered();

private:
    Ui::MainWindow *ui;
    int FreeOthersWidget(void *widget);
    IspIoController *ispiocontroller = NULL;
    Isp485 *isp485 = NULL;
    IspCollector *ispcollector = NULL;
    IspIotProgram *ispiotprogram = NULL;
    Rj45IotProgram *rj45iotprogram = NULL;
    StmIsp *stmisp = NULL;
    TkmIsp *tkmisp = NULL;
    UartAssist *uartassist = NULL;
    NetAssist *netassist = NULL;
    Smartbuilding22 *smartbuilding22 = NULL;
    Tkm300Config *tkm300config = NULL;
};

#endif // MAINWINDOW_H
