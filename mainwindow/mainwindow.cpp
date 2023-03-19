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
    if (ispiocontroller && ispiocontroller != widget) {
        if (ispiocontroller->GetBtnStatus()) {
            return -1;
        }
        delete ispiocontroller;
        ispiocontroller = NULL;
    }
    if (isp485 && isp485 != widget) {
        if (isp485->GetBtnStatus()) {
            return -1;
        }
        delete isp485;
        isp485 = NULL;
    }
    if (ispcollector && ispcollector != widget) {
        if (ispcollector->GetBtnStatus()) {
            return -1;
        }
        delete ispcollector;
        ispcollector = NULL;
    }
    if (ispiotprogram && ispiotprogram != widget) {
        if (ispiotprogram->GetBtnStatus()) {
            return -1;
        }
        delete ispiotprogram;
        ispiotprogram = NULL;
    }
    if (rj45iotprogram && rj45iotprogram != widget) {
        if (rj45iotprogram->GetBtnStatus()) {
            return -1;
        }
        delete rj45iotprogram;
        rj45iotprogram = NULL;
    }
    if (stmisp && stmisp != widget) {
        if (stmisp->GetBtnStatus()) {
            return -1;
        }
        delete stmisp;
        stmisp = NULL;
    }
    if (tkmisp && tkmisp != widget) {
        if (tkmisp->GetBtnStatus()) {
            return -1;
        }
        delete tkmisp;
        tkmisp = NULL;
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
    if (smartbuilding22 && smartbuilding22 != widget) {
        if (smartbuilding22->GetBtnStatus()) {
            return -1;
        }
        delete smartbuilding22;
        smartbuilding22 = NULL;
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
    if (!ispiocontroller) {
        ispiocontroller = new IspIoController(ui->centralWidget);
        int w = ispiocontroller->width();
        int h = ispiocontroller->height();
        setFixedSize(w, h + 23);
        ispiocontroller->show();
    }
}

void MainWindow::on_actionIsp485GateWay_triggered () {
    if (FreeOthersWidget(isp485)) {
        return;
    }
    if (!isp485) {
        isp485 = new Isp485(ui->centralWidget);
        int w = isp485->width();
        int h = isp485->height();
        setFixedSize(w, h + 23);
        isp485->show();
    }
}

void MainWindow::on_actionIspCollector_triggered () {
    if (FreeOthersWidget(ispcollector)) {
        return;
    }
    if (!ispcollector) {
        ispcollector = new IspCollector(ui->centralWidget);
        int w = ispcollector->width();
        int h = ispcollector->height();
        setFixedSize(w, h + 23);
        ispcollector->show();
    }
}

void MainWindow::on_actionIspIotProgram_triggered () {
    if (FreeOthersWidget(ispiotprogram)) {
        return;
    }
    if (!ispiotprogram) {
        ispiotprogram = new IspIotProgram(ui->centralWidget);
        int w = ispiotprogram->width();
        int h = ispiotprogram->height();
        setFixedSize(w, h + 23);
        ispiotprogram->show();
    }
}

void MainWindow::on_actionRj45IotProgram_triggered () {
    if (FreeOthersWidget(rj45iotprogram)) {
        return;
    }
    if (!rj45iotprogram) {
        rj45iotprogram = new Rj45IotProgram(ui->centralWidget);
        int w = rj45iotprogram->width();
        int h = rj45iotprogram->height();
        setFixedSize(w, h + 23);
        rj45iotprogram->show();
    }
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

void MainWindow::on_actiontkm_triggered() {
    if (FreeOthersWidget(tkmisp)) {
        return;
    }
    if (!tkmisp) {
        tkmisp = new TkmIsp(ui->centralWidget);
        int w = tkmisp->width();
        int h = tkmisp->height();
        setFixedSize(w, h + 23);
        tkmisp->show();
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

void MainWindow::on_actionSmartbuilding22_triggered() {
    if (FreeOthersWidget(smartbuilding22)) {
        return;
    }
    if (!smartbuilding22) {
        smartbuilding22 = new Smartbuilding22(ui->centralWidget);
        int w = smartbuilding22->width();
        int h = smartbuilding22->height();
        setFixedSize(w, h + 23);
        smartbuilding22->show();
    }
}
