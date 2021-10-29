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
    delete ui;
}

void MainWindow::on_actionIoController_triggered()
{
    if (ispiocontroller == NULL) {
        ispiocontroller = new IspIoController(ui->centralWidget);
        int w = ispiocontroller->width();
        int h = ispiocontroller->height();
        setFixedSize(w, h + 23);
        ispiocontroller->show();
    }
}
