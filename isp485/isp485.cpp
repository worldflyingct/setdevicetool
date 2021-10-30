#include "isp485.h"
#include "ui_isp485.h"

Isp485::Isp485(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Isp485)
{
    ui->setupUi(this);
    groupRadio = new QButtonGroup(this);
    groupRadio->addButton(ui->mqttradio, 0);
    groupRadio->addButton(ui->tcpradio, 1);
    groupRadio->addButton(ui->udpradio, 2);
    ui->mqttradio->setChecked(true);
}

Isp485::~Isp485()
{
    delete ui;
}
