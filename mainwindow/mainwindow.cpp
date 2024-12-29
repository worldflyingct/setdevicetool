#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow (QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setFixedSize(400, 300);
}

MainWindow::~MainWindow () {
    FreeOthersWidget(NULL);
    delete ui;
}

int MainWindow::FreeOthersWidget (void *widget) {
    if (widget != ui->label) {
        ui->label->setVisible(false);
    }
    if (stmisp && stmisp != widget) {
        if (stmisp->GetBtnStatus()) {
            return -1;
        }
        delete stmisp;
        stmisp = NULL;
    }
    if (uartassist && uartassist != widget) {
        if (uartassist->GetBtnStatus()) {
            return -1;
        }
        delete uartassist;
        uartassist = NULL;
    }
    if (netassist && netassist != widget) {
        if (netassist->GetBtnStatus()) {
            return -1;
        }
        delete netassist;
        netassist = NULL;
    }
    if (stmisp && stmisp != widget) {
        if (stmisp->GetBtnStatus()) {
            return -1;
        }
        delete stmisp;
        stmisp = NULL;
    }
    return 0;
}

void MainWindow::on_actionAboutQt_triggered () {
    QMessageBox::aboutQt(NULL, "关于Qt");
}

void MainWindow::on_actionAbout_triggered () {
    if (FreeOthersWidget(ui->label)) {
        return;
    }
    setFixedSize(400, 300);
    ui->label->setVisible(true);
}

void MainWindow::on_actionStmIsp_triggered () {
    if (FreeOthersWidget(stmisp)) {
        return;
    }
    if (!stmisp) {
        stmisp = new StmIsp(ui->centralWidget);
        int w = stmisp->width();
        int h = stmisp->height();
        setFixedSize(w, h + 23);
        stmisp->show();
    }
}

void MainWindow::on_actionUartAssist_triggered () {
    if (FreeOthersWidget(uartassist)) {
        return;
    }
    if (!uartassist) {
        uartassist = new UartAssist(ui->centralWidget);
        int w = uartassist->width();
        int h = uartassist->height();
        setFixedSize(w, h + 23);
        uartassist->show();
    }
}

void MainWindow::on_actionNetAssist_triggered () {
    if (FreeOthersWidget(netassist)) {
        return;
    }
    if (!netassist) {
        netassist = new NetAssist(ui->centralWidget);
        int w = netassist->width();
        int h = netassist->height();
        setFixedSize(w, h + 23);
        netassist->show();
    }
}

void MainWindow::on_actionRs485Server_triggered () {
    if (FreeOthersWidget(rs485serialserver)) {
        return;
    }
    if (!rs485serialserver) {
        rs485serialserver = new Rs485SerialServer(ui->centralWidget);
        int w = rs485serialserver->width();
        int h = rs485serialserver->height();
        setFixedSize(w, h + 23);
        rs485serialserver->show();
    }
}
