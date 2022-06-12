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
    if (ispiocontroller != NULL) {
        if (ispiocontroller == widget) {
            return -1;
        }
        if (ispiocontroller->GetBtnStatus()) {
            return -2;
        }
        delete ispiocontroller;
        ispiocontroller = NULL;
    }
    if (isp485 != NULL) {
        if (isp485 == widget) {
            return -1;
        }
        if (isp485->GetBtnStatus()) {
            return -2;
        }
        delete isp485;
        isp485 = NULL;
    }
    if (ispcollector != NULL) {
        if (ispcollector == widget) {
            return -1;
        }
        if (ispcollector->GetBtnStatus()) {
            return -2;
        }
        delete ispcollector;
        ispcollector = NULL;
    }
    if (ispiotprogram != NULL) {
        if (ispiotprogram == widget) {
            return -1;
        }
        if (ispiotprogram->GetBtnStatus()) {
            return -2;
        }
        delete ispiotprogram;
        ispiotprogram = NULL;
    }
    if (stmisp != NULL) {
        if (stmisp == widget) {
            return -1;
        }
        if (stmisp->GetBtnStatus()) {
            return -2;
        }
        delete stmisp;
        stmisp = NULL;
    }
    if (uartassist != NULL) {
        if (uartassist == widget) {
            return -1;
        }
        if (uartassist->GetBtnStatus()) {
            return -2;
        }
        delete uartassist;
        uartassist = NULL;
    }
    if (netassist != NULL) {
        if (netassist == widget) {
            return -1;
        }
        if (netassist->GetBtnStatus()) {
            return -2;
        }
        delete netassist;
        netassist = NULL;
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

void MainWindow::on_actionIspIoController_triggered () {
    if (FreeOthersWidget(ispiocontroller)) {
        return;
    }
    if (ispiocontroller == NULL) {
        ispiocontroller = new IspIoController(ui->centralWidget);
    }
    int w = ispiocontroller->width();
    int h = ispiocontroller->height();
    setFixedSize(w, h + 23);
    ispiocontroller->show();
}

void MainWindow::on_actionIsp485GateWay_triggered () {
    if (FreeOthersWidget(isp485)) {
        return;
    }
    if (isp485 == NULL) {
        isp485 = new Isp485(ui->centralWidget);
    }
    int w = isp485->width();
    int h = isp485->height();
    setFixedSize(w, h + 23);
    isp485->show();
}

void MainWindow::on_actionIspCollector_triggered () {
    if (FreeOthersWidget(ispcollector)) {
        return;
    }
    if (ispcollector == NULL) {
        ispcollector = new IspCollector(ui->centralWidget);
    }
    int w = ispcollector->width();
    int h = ispcollector->height();
    setFixedSize(w, h + 23);
    ispcollector->show();
}

void MainWindow::on_actionIspIotProgram_triggered () {
    if (FreeOthersWidget(ispiotprogram)) {
        return;
    }
    if (ispiotprogram == NULL) {
        ispiotprogram = new IspIotProgram(ui->centralWidget);
    }
    int w = ispiotprogram->width();
    int h = ispiotprogram->height();
    setFixedSize(w, h + 23);
    ispiotprogram->show();
}

void MainWindow::on_actionStmIsp_triggered () {
    if (FreeOthersWidget(stmisp)) {
        return;
    }
    if (stmisp == NULL) {
        stmisp = new StmIsp(ui->centralWidget);
    }
    int w = stmisp->width();
    int h = stmisp->height();
    setFixedSize(w, h + 23);
    stmisp->show();
}

void MainWindow::on_actionUartAssist_triggered () {
    if (FreeOthersWidget(uartassist)) {
        return;
    }
    if (uartassist == NULL) {
        uartassist = new UartAssist(ui->centralWidget);
    }
    int w = uartassist->width();
    int h = uartassist->height();
    setFixedSize(w, h + 23);
    uartassist->show();
}

void MainWindow::on_actionNetAssist_triggered () {
    if (FreeOthersWidget(netassist)) {
        return;
    }
    if (netassist == NULL) {
        netassist = new NetAssist(ui->centralWidget);
    }
    int w = netassist->width();
    int h = netassist->height();
    setFixedSize(w, h + 23);
    netassist->show();
}
