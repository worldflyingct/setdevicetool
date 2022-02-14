#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setFixedSize(400, 300);
}

MainWindow::~MainWindow() {
    FreeOthersWidget(NULL);
    delete ui;
}

void MainWindow::FreeOthersWidget(void *widget) {
    if (widget != ui->label) {
        ui->label->setVisible(false);
    }
    if (ispiocontroller != NULL && ispiocontroller != widget) {
        delete ispiocontroller;
        ispiocontroller = NULL;
    }
    if (isp485 != NULL && isp485 != widget) {
        delete isp485;
        isp485 = NULL;
    }
    if (isplocation != NULL && isplocation != widget) {
        delete isplocation;
        isplocation = NULL;
    }
    if (ispiotprogram != NULL && ispiotprogram != widget) {
        delete ispiotprogram;
        ispiotprogram = NULL;
    }
    if (stmisp != NULL && stmisp != widget) {
        delete stmisp;
        stmisp = NULL;
    }
}

void MainWindow::on_actionAbout_triggered() {
    FreeOthersWidget(ui->label);
    setFixedSize(400, 300);
    ui->label->setVisible(true);
}

void MainWindow::on_actionIoController_triggered() {
    FreeOthersWidget(ispiocontroller);
    if (ispiocontroller == NULL) {
        ispiocontroller = new IspIoController(ui->centralWidget);
    }
    int w = ispiocontroller->width();
    int h = ispiocontroller->height();
    setFixedSize(w, h + 23);
    ispiocontroller->show();
}

void MainWindow::on_action485GateWay_triggered() {
    FreeOthersWidget(isp485);
    if (isp485 == NULL) {
        isp485 = new Isp485(ui->centralWidget);
    }
    int w = isp485->width();
    int h = isp485->height();
    setFixedSize(w, h + 23);
    isp485->show();
}

void MainWindow::on_actionLocation_triggered() {
    FreeOthersWidget(isplocation);
    if (isplocation == NULL) {
        isplocation = new IspLocation(ui->centralWidget);
    }
    int w = isplocation->width();
    int h = isplocation->height();
    setFixedSize(w, h + 23);
    isplocation->show();
}

void MainWindow::on_actionIspiotprogram_triggered() {
    FreeOthersWidget(ispiotprogram);
    if (ispiotprogram == NULL) {
        ispiotprogram = new IspIotProgram(ui->centralWidget);
    }
    int w = ispiotprogram->width();
    int h = ispiotprogram->height();
    setFixedSize(w, h + 23);
    ispiotprogram->show();
}

void MainWindow::on_actionStmIsp_triggered() {
    FreeOthersWidget(stmisp);
    if (stmisp == NULL) {
        stmisp = new StmIsp(ui->centralWidget);
    }
    int w = stmisp->width();
    int h = stmisp->height();
    setFixedSize(w, h + 23);
    stmisp->show();
}
