#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ispiocontroller/ispiocontroller.h"
#include "isp485/isp485.h"
#include "isplocation/isplocation.h"
#include "ispiotprogram/ispiotprogram.h"
#include "stmisp/stmisp.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionAbout_triggered();
    void on_actionIspIoController_triggered();
    void on_actionIsp485GateWay_triggered();
    void on_actionIspLocation_triggered();
    void on_actionIspIotProgram_triggered();
    void on_actionStmIsp_triggered();

private:
    Ui::MainWindow *ui;
    int FreeOthersWidget(void *widget);
    IspIoController *ispiocontroller = NULL;
    Isp485 *isp485 = NULL;
    IspLocation *isplocation = NULL;
    IspIotProgram * ispiotprogram = NULL;
    StmIsp * stmisp = NULL;
};

#endif // MAINWINDOW_H
