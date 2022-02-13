#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ispiocontroller/ispiocontroller.h"
#include "isp485/isp485.h"
#include "isplocation/isplocation.h"
#include "ispiotprogram/ispiotprogram.h"
#include "stm32isp/stm32isp.h"

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
    void on_actionIoController_triggered();
    void on_action485GateWay_triggered();
    void on_actionLocation_triggered();
    void on_actionIspiotprogram_triggered();
    void on_actionStm32Isp_triggered();

private:
    Ui::MainWindow *ui;
    void FreeOthersWidget(void *widget);
    IspIoController *ispiocontroller = NULL;
    Isp485 *isp485 = NULL;
    IspLocation *isplocation = NULL;
    IspIotProgram * ispiotprogram = NULL;
    Stm32Isp * stm32isp = NULL;
};

#endif // MAINWINDOW_H
