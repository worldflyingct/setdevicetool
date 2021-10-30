#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setFixedSize(400, 300);
}

MainWindow::~MainWindow()
{
    FreeOthersWidget(NULL);
    delete ui;
}

void MainWindow::FreeOthersWidget(void *widget)
{
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
}

void MainWindow::on_actionIoController_triggered()
{
    FreeOthersWidget(ispiocontroller);
    if (ispiocontroller == NULL) {
        ispiocontroller = new IspIoController(ui->centralWidget);
        int w = ispiocontroller->width();
        int h = ispiocontroller->height();
        setFixedSize(w, h + 23);
        ispiocontroller->show();
    }
}

void MainWindow::on_action485GateWay_triggered()
{
    FreeOthersWidget(isp485);
    if (isp485 == NULL) {
        isp485 = new Isp485(ui->centralWidget);
        int w = isp485->width();
        int h = isp485->height();
        setFixedSize(w, h + 23);
        isp485->show();
    }
}

void MainWindow::on_actionAbout_triggered()
{
    FreeOthersWidget(ui->label);
    setFixedSize(400, 300);
    ui->label->setVisible(true);
}
