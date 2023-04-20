#include "tkg300config.h"
#include "ui_tkg300config.h"

#define AT_GETDHCP          0x01
#define AT_GETGWIP          0x02
#define AT_GETDNS1          0x03
#define AT_GETMASK          0x04
#define AT_GETGW            0x05
#define AT_GETMQTTSERVER    0x06
#define AT_GETMQTTUSER      0x07
#define AT_GETSN            0x08
#define AT_SETDHCP          0x11
#define AT_SETGWIP          0x12
#define AT_SETDNS1          0x13
#define AT_SETMASK          0x14
#define AT_SETGW            0x15
#define AT_SETMQTTSERVER    0x16
#define AT_SETMQTTUSER      0x17

Tkg300Config::Tkg300Config(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Tkg300Config) {
    ui->setupUi(this);
    GetComList();
    ipgroup.addButton(ui->dhcp, 0);
    ipgroup.addButton(ui->staticip, 1);
    ui->dhcp->setChecked(true);
    on_dhcp_clicked();
}

Tkg300Config::~Tkg300Config() {
    delete ui;
}

// 获取可用的串口号
void Tkg300Config::GetComList () {
    QComboBox *c = ui->com;
    c->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        c->addItem(info.portName() + " " + info.description());
    }
}

void Tkg300Config::on_refresh_clicked () {
    GetComList();
}

void Tkg300Config::TimerOutEvent () {
    if (btnStatus == 1) {
        ui->tips->setText("设备响应超时");
    } else if (btnStatus == 2) {
        ui->tips->setText("数据接收超时");
    }
    CloseSerial();
}

void Tkg300Config::SerialErrorEvent () {
    ui->tips->setText("串口错误");
    CloseSerial();
    GetComList();
}

int Tkg300Config::GetBtnStatus () {
    return btnStatus;
}

void Tkg300Config::ReadSerialData () {
    QByteArray arr = serial.readAll();
    int len = arr.length();
    char *c = arr.data();
    memcpy(serialReadBuff+bufflen, c, len);
    bufflen += len;
    serialReadBuff[bufflen] = '\0';
    if (chipstep == AT_GETDHCP) {
        if (!strstr((char*)serialReadBuff, "AT_OK\r\n")) {
            return;
        }
        char *dhcp = strstr((char*)serialReadBuff, ":");
        if (dhcp == NULL || dhcp[1] == '\0') {
            return;
        }
        if (dhcp[1] == '1') {
            ui->dhcp->setChecked(true);
            on_dhcp_clicked();
        } else if (dhcp[1] == '0') {
            ui->staticip->setChecked(true);
            on_staticip_clicked();
        } else {
            ui->tips->setText("获取dhcp状态失败");
            CloseSerial();
            return;
        }
        ui->tips->setText("获取dhcp状态成功，开始获取ip");
        chipstep = AT_GETGWIP;
        bufflen = 0;
        serial.write("AT+GWIP=?\r\n", 11);
    } else if (chipstep == AT_GETGWIP) {
        if (!strstr((char*)serialReadBuff, "AT_OK\r\n")) {
            return;
        }
        char *gwip = strstr((char*)serialReadBuff, ":");
        if (gwip == NULL) {
            return;
        }
        int i;
        for(i = 1 ; gwip[i] != '\r' ; i++);
        gwip[i] = '\0';
        ui->ip->setText(gwip+1);
        ui->tips->setText("获取ip状态成功，开始获取netmask");
        chipstep = AT_GETMASK;
        bufflen = 0;
        serial.write("AT+MASK=?\r\n", 11);
    } else if (chipstep == AT_GETMASK) {
        if (!strstr((char*)serialReadBuff, "AT_OK\r\n")) {
            return;
        }
        char *mask = strstr((char*)serialReadBuff, ":");
        if (mask == NULL) {
            return;
        }
        int i;
        for(i = 1 ; mask[i] != '\r' ; i++);
        mask[i] = '\0';
        ui->netmask->setText(mask+1);
        ui->tips->setText("获取netmask状态成功，开始获取gateway");
        chipstep = AT_GETGW;
        bufflen = 0;
        serial.write("AT+GW=?\r\n", 9);
    } else if (chipstep == AT_GETGW) {
        if (!strstr((char*)serialReadBuff, "AT_OK\r\n")) {
            return;
        }
        char *gw = strstr((char*)serialReadBuff, ":");
        if (gw == NULL) {
            return;
        }
        int i;
        for(i = 1 ; gw[i] != '\r' ; i++);
        gw[i] = '\0';
        ui->gateway->setText(gw+1);
        ui->tips->setText("获取gateway状态成功，开始获取dns");
        chipstep = AT_GETDNS1;
        bufflen = 0;
        serial.write("AT+DNS1=?\r\n", 11);
    } else if (chipstep == AT_GETDNS1) {
        if (!strstr((char*)serialReadBuff, "AT_OK\r\n")) {
            return;
        }
        char *dns1 = strstr((char*)serialReadBuff, ":");
        if (dns1 == NULL) {
            return;
        }
        int i;
        for(i = 1 ; dns1[i] != '\r' ; i++);
        dns1[i] = '\0';
        ui->dns->setText(dns1+1);
        ui->tips->setText("获取dns状态成功，开始获取mqtturl");
        chipstep = AT_GETMQTTSERVER;
        bufflen = 0;
        serial.write("AT+SERVER=?\r\n", 13);
    } else if (chipstep == AT_GETMQTTSERVER) {
        if (!strstr((char*)serialReadBuff, "AT_OK\r\n")) {
            return;
        }
        char *mqttserver = strstr((char*)serialReadBuff, ":");
        if (mqttserver == NULL) {
            return;
        }
        int i;
        for(i = 1 ; mqttserver[i] != '\r' ; i++);
        mqttserver[i] = '\0';
        char *mqttserverport = strstr(mqttserver+i+1, ":");
        for(i = 1 ; mqttserverport[i] != '\r' ; i++);
        mqttserverport[i] = '\0';
        char buff[64];
        sprintf(buff, "tcp://%s:%s", mqttserver+1, mqttserverport+1);
        ui->mqtturl->setText(buff);
        ui->tips->setText("获取mqtturl状态成功，开始获取mqttuser与mqttpass");
        chipstep = AT_GETMQTTUSER;
        bufflen = 0;
        serial.write("AT+MQTTUSR=?\r\n", 14);
    } else if (chipstep == AT_GETMQTTUSER) {
        if (!strstr((char*)serialReadBuff, "AT_OK\r\n")) {
            return;
        }
        char *mqttuser = strstr((char*)serialReadBuff, ":");
        if (mqttuser == NULL) {
            return;
        }
        int i;
        for(i = 1 ; mqttuser[i] != '\r' ; i++);
        mqttuser[i] = '\0';
        ui->mqttuser->setText(mqttuser+1);
        char *mqttpass = strstr(mqttuser+i+1, ":");
        for(i = 1 ; mqttpass[i] != '\r' ; i++);
        mqttpass[i] = '\0';
        ui->mqttpass->setText(mqttpass+1);
        ui->tips->setText("获取mqtturl状态成功，开始获取设备sn");
        chipstep = AT_GETSN;
        bufflen = 0;
        serial.write("AT+GWMAC=?\r\n", 12);
    } else if (chipstep == AT_GETSN) {
        if (!strstr((char*)serialReadBuff, "AT_OK\r\n")) {
            return;
        }
        char *sn = strstr((char*)serialReadBuff, ":");
        if (sn == NULL) {
            return;
        }
        int i;
        for(i = 1 ; sn[i] != '\r' ; i++);
        sn[i] = '\0';
        char buff[128];
        sprintf(buff, "获取设备信息完成\r\nSN:%s", sn+1);
        ui->tips->setText(buff);
        CloseSerial();
    } else if (chipstep == AT_SETDHCP) {
        if (!strstr((char*)serialReadBuff, "AT_OK\r\n")) {
            return;
        }
        if (ui->dhcp->isChecked()) {
            ui->tips->setText("设置设备dhcp功能成功，开始设置设备mqttserver");
            chipstep = AT_SETMQTTSERVER;
            bufflen = 0;
            char buff[64];
            char *mqtturl = ui->mqtturl->text().toUtf8().data()+6;
            int i;
            for (i = 0 ; mqtturl[i] != ':' ; i++);
            mqtturl[i] = ',';
            int len = sprintf(buff, "AT+SERVER=%s\r\n", mqtturl);
            serial.write(buff, len);
        } else {
            ui->tips->setText("设置设备dhcp功能成功，开始设置设备ip");
            chipstep = AT_SETGWIP;
            bufflen = 0;
            char buff[64];
            int len = sprintf(buff, "AT+GWIP=%s\r\n", ui->ip->text().toUtf8().data());
            serial.write(buff, len);
        }
    } else if (chipstep == AT_SETGWIP) {
        if (!strstr((char*)serialReadBuff, "AT_OK\r\n")) {
            return;
        }
        ui->tips->setText("设置设备ip成功，开始设置设备netmask");
        chipstep = AT_SETMASK;
        bufflen = 0;
        char buff[64];
        int len = sprintf(buff, "AT+MASK=%s\r\n", ui->netmask->text().toUtf8().data());
        serial.write(buff, len);
    } else if (chipstep == AT_SETMASK) {
        if (!strstr((char*)serialReadBuff, "AT_OK\r\n")) {
            return;
        }
        ui->tips->setText("设置设备netmask成功，开始设置设备gateway");
        chipstep = AT_SETGW;
        bufflen = 0;
        char buff[64];
        int len = sprintf(buff, "AT+GW=%s\r\n", ui->gateway->text().toUtf8().data());
        serial.write(buff, len);
    } else if (chipstep == AT_SETGW) {
        if (!strstr((char*)serialReadBuff, "AT_OK\r\n")) {
            return;
        }
        ui->tips->setText("设置设备gateway成功，开始设置设备dns");
        chipstep = AT_SETDNS1;
        bufflen = 0;
        char buff[64];
        int len = sprintf(buff, "AT+DNS1=%s\r\n", ui->dns->text().toUtf8().data());
        serial.write(buff, len);
    } else if (chipstep == AT_SETDNS1) {
        if (!strstr((char*)serialReadBuff, "AT_OK\r\n")) {
            return;
        }
        ui->tips->setText("设置设备dns成功，开始设置设备mqttserver");
        chipstep = AT_SETMQTTSERVER;
        bufflen = 0;
        char buff[64];
        char *mqtturl = ui->mqtturl->text().toUtf8().data()+6;
        int i;
        for (i = 0 ; mqtturl[i] != ':' ; i++);
        mqtturl[i] = ',';
        int len = sprintf(buff, "AT+SERVER=%s\r\n", mqtturl);
        serial.write(buff, len);
    } else if (chipstep == AT_SETMQTTSERVER) {
        if (!strstr((char*)serialReadBuff, "AT_OK\r\n")) {
            return;
        }
        ui->tips->setText("设置设备mqttserver成功，开始设置设备mqttuser与mqttpass");
        chipstep = AT_SETMQTTUSER;
        bufflen = 0;
        char buff[128];
        int len = sprintf(buff, "AT+MQTTUSR=%s,%s\r\n", ui->mqttuser->text().toUtf8().data(), ui->mqttpass->text().toUtf8().data());
        serial.write(buff, len);
    } else if (chipstep == AT_SETMQTTUSER) {
        if (!strstr((char*)serialReadBuff, "AT_OK\r\n")) {
            return;
        }
        ui->tips->setText("设置完成");
        CloseSerial();
    }
    timer.stop();
    timer.start(1000);
}

int Tkg300Config::OpenSerial () {
    char comname[12];
    sscanf(ui->com->currentText().toUtf8().data(), "%s ", comname);
    serial.setPortName(comname);
    serial.setBaudRate(QSerialPort::Baud115200);
    serial.setParity(QSerialPort::NoParity);
    serial.setDataBits(QSerialPort::Data8);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::NoFlowControl);
    if (!serial.open(QIODevice::ReadWrite)) {
        return -1;
    }
    serial.clear();
    connect(&timer, SIGNAL(timeout()), this, SLOT(TimerOutEvent()));
    connect(&serial, SIGNAL(readyRead()), this, SLOT(ReadSerialData()));
    connect(&serial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(SerialErrorEvent()));
    timer.start(1000);
    return 0;
}

void Tkg300Config::CloseSerial () {
    disconnect(&timer, 0, 0, 0);
    timer.stop();
    disconnect(&serial, 0, 0, 0);
    serial.close();
    bufflen = 0;
    btnStatus = 0;
}

void Tkg300Config::on_setconfig_clicked() {
    if (btnStatus) {
        ui->tips->setText("用户终止操作");
        CloseSerial();
        return;
    }
    if (memcmp(ui->mqtturl->text().toUtf8().data(), "tcp://", 6)) {
        ui->tips->setText("mqtturl格式错误");
        return;
    }
    if (OpenSerial()) {
        ui->tips->setText("串口打开失败");
        return;
    }
    btnStatus = 2;
    ui->tips->setText("开始设置设备dhcp功能");
    chipstep = AT_SETDHCP;
    bufflen = 0;
    char buff[64];
    int dhcp = ui->dhcp->isChecked() ? 1 : 0;
    int len = sprintf(buff, "AT+DHCP=%d\r\n", dhcp);
    serial.write(buff, len);
}

void Tkg300Config::on_getconfig_clicked() {
    if (btnStatus) {
        ui->tips->setText("用户终止操作");
        CloseSerial();
        return;
    }
    if (OpenSerial()) {
        ui->tips->setText("串口打开失败");
        return;
    }
    btnStatus = 2;
    ui->tips->setText("开始获取dhcp状态");
    chipstep = AT_GETDHCP;
    bufflen = 0;
    serial.write("AT+DHCP=?\r\n", 11);
}

void Tkg300Config::on_dhcp_clicked() {
    ui->ip->setEnabled(false);
    ui->netmask->setEnabled(false);
    ui->gateway->setEnabled(false);
    ui->dns->setEnabled(false);
}

void Tkg300Config::on_staticip_clicked() {
    ui->ip->setEnabled(true);
    ui->netmask->setEnabled(true);
    ui->gateway->setEnabled(true);
    ui->dns->setEnabled(true);
}
