#include "isp485.h"
#include "ui_isp485.h"
// cjson库
#include "common/cjson.h"
// 公共函数库
#include "common/common.h"

Isp485::Isp485(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Isp485)
{
    ui->setupUi(this);
    groupRadio = new QButtonGroup(this);
    groupRadio->addButton(ui->mqttradio, 0);
    groupRadio->addButton(ui->otherradio, 1);
    ui->mqttradio->setChecked(true);
    connect(ui->mqttradio, SIGNAL(clicked(bool)), this, SLOT(ModeChanged()));
    connect(ui->otherradio, SIGNAL(clicked(bool)), this, SLOT(ModeChanged()));
    GetComList();
}

Isp485::~Isp485()
{
    disconnect(ui->mqttradio, 0, 0, 0);
    disconnect(ui->otherradio, 0, 0, 0);
    delete groupRadio;
    delete ui;
}

// 获取可用的串口号
void Isp485::GetComList ()
{
    QComboBox *c = ui->com;
    for (int a = c->count() ; a > 0 ; a--)
    {
        c->removeItem(a-1);
    }
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        c->addItem(info.portName() + " " + info.description());
    }
}

void Isp485::on_refresh_clicked()
{
    this->GetComList();
}

void Isp485::ModeChanged()
{
    switch(groupRadio->checkedId()) {
        case 0:
            ui->user->setEnabled(true);
            ui->pass->setEnabled(true);
            ui->sendtopic->setEnabled(true);
            ui->receivetopic->setEnabled(true);
            break;
        case 1:
            ui->user->setEnabled(false);
            ui->pass->setEnabled(false);
            ui->sendtopic->setEnabled(false);
            ui->receivetopic->setEnabled(false);
            break;
    }
}

void Isp485::TimerOutEvent()
{
    if (bufflen) {
        HandleSerialData();
        bufflen = 0;
    } else {
        ui->tips->setText("数据接收超时");
        CloseSerial();
        btnStatus = 0;
    }
}

void Isp485::ReadSerialData()
{
    QByteArray arr = serial.readAll();
    int len = arr.length();
    char *c = arr.data();
    memcpy(serialReadBuff+bufflen, c, len);
    bufflen += len;
    timer.stop();
    timer.start(500);
}

int Isp485::OpenSerial()
{
    char comname[12];
    sscanf(ui->com->currentText().toStdString().c_str(), "%s ", comname);
    serial.setPortName(comname);
    serial.setBaudRate(QSerialPort::Baud115200);
    serial.setParity(QSerialPort::NoParity);
    serial.setDataBits(QSerialPort::Data8);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::NoFlowControl);
    if (!serial.open(QIODevice::ReadWrite))
    {
        return -1;
    }
    connect(&timer, SIGNAL(timeout()), this, SLOT(TimerOutEvent()));
    connect(&serial, SIGNAL(readyRead()), this, SLOT(ReadSerialData()));
    timer.start(500);
    return 0;
}

void Isp485::CloseSerial()
{
    disconnect(&timer, 0, 0, 0);
    timer.stop();
    disconnect(&serial, 0, 0, 0);
    serial.close();
}

void Isp485::on_setconfig_clicked()
{
    ui->tips->setText("");
    if (OpenSerial()) {
        ui->tips->setText("串口打开失败");
        return;
    }
    ui->tips->setText("串口打开成功");
    // 下面的不同项目可能不同，另外，0.5s后没有收到设备返回就会提示失败。
    int radio = groupRadio->checkedId();
    if (radio == 1) { // mqtt模式
        std::string url = ui->url->text().toStdString();
        if (url.length() == 0) {
            ui->tips->setText("URL为空");
            CloseSerial();
            return;
        }
        std::string user = ui->user->text().toStdString();
        if (user.length() == 0) {
            ui->tips->setText("USER为空");
            CloseSerial();
            return;
        }
        std::string pass = ui->pass->text().toStdString();
        if (pass.length() == 0) {
            ui->tips->setText("PASS为空");
            CloseSerial();
            return;
        }
        std::string sendtopic = ui->sendtopic->text().toStdString();
        if (sendtopic.length() == 0) {
            ui->tips->setText("发送topic为空");
            CloseSerial();
            return;
        }
        std::string receivetopic = ui->receivetopic->text().toStdString();
        if (receivetopic.length() == 0) {
            ui->tips->setText("接收topic为空");
            CloseSerial();
            return;
        }
    } else if (radio == 0) { // tcp或是udp模式
        std::string curl = ui->url->text().toStdString();
        int len = curl.length();
        if (len == 0) {
            ui->tips->setText("URL为空");
            CloseSerial();
            return;
        }
        char url[len];
        memcpy(url, curl.c_str(), len);
        char *protocol;
        char *serveraddr;
        unsigned short port;
        int res = urldecode (url, len, &protocol, &serveraddr, &port);
        qDebug("res:%d, protocol:%s, serveraddr:%s, port:%u", res, protocol, serveraddr, port);
    }
}

void Isp485::HandleSerialData()
{

}
