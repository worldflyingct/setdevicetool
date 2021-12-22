#include "isplocation.h"
#include "ui_isplocation.h"

IspLocation::IspLocation(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IspLocation)
{
    ui->setupUi(this);
    GetComList();
}

IspLocation::~IspLocation()
{
    delete ui;
}

// 获取可用的串口号
void IspLocation::GetComList () {
/*
    QComboBox *c = ui->com;
    for (int a = c->count() ; a > 0 ; a--) {
        c->removeItem(a-1);
    }
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        c->addItem(info.portName() + " " + info.description());
    }
*/
    QComboBox *c = ui->com;
    c->addItem("COM3 Serial Port");
    c->addItem("COM4 CH340 Port");
    c->addItem("COM5 PL2303 Port");
}

void IspLocation::on_refresh_clicked()
{
    this->GetComList();
}

void IspLocation::on_setconfig_clicked()
{
    std::string cserverurl = ui->serverurl->text().toStdString();
    int len = cserverurl.length();
    if (len == 0) {
        ui->tips->setText("服务器地址为空");
        return;
    }
    char url[len];
    memcpy(url, cserverurl.c_str(), len);
    if (memcmp(url, "coap://", 7) && memcmp(url, "udp://", 6)) {
        ui->tips->setText("只支持coap与udp协议");
        return;
    }
    int day = ui->day->value();
    int hour = ui->hour->value();
    int minute = ui->minute->value();
    int second = ui->day->value();
    int totalsecond = 24*60*60*day + 60*60*hour + 60*minute + second;
    if (!totalsecond) {
        ui->tips->setText("时间间隔不能为0");
        return;
    }
    ui->tips->setText("设置成功");
}

void IspLocation::on_getconfig_clicked()
{
    ui->serverurl->setText("coap://testserveraddr:5683");
    ui->day->setValue(0);
    ui->hour->setValue(4);
    ui->minute->setValue(0);
    ui->second->setValue(0);
    ui->tips->setText("设备SN:1234567890");
}
