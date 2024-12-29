#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "stmisp/stmisp.h"
#include "uartassist/uartassist.h"
#include "netassist/netassist.h"
#include "rs485serialserver/rs485serialserver.h"

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
    void on_actionStmIsp_triggered();
    void on_actionUartAssist_triggered();
    void on_actionNetAssist_triggered();
    void on_actionRs485Server_triggered();

private:
    Ui::MainWindow *ui;
    int FreeOthersWidget(void *widget);
    StmIsp *stmisp = NULL;
    UartAssist *uartassist = NULL;
    NetAssist *netassist = NULL;
    Rs485SerialServer *rs485serialserver = NULL;
};

#endif // MAINWINDOW_H
