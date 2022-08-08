#include "rj45iotprogram.h"
#include "ui_rj45iotprogram.h"
// 公共函数库
#include "common/common.h"

Rj45IotProgram::Rj45IotProgram(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Rj45IotProgram) {
    ui->setupUi(this);
    GetComList();
    ipgroup.addButton(ui->dhcp, 0);
    ipgroup.addButton(ui->staticip, 1);
    modegroup.addButton(ui->rj45mode, 0);
    modegroup.addButton(ui->wifimode, 1);
    ui->dhcp->setChecked(true);
    ui->rj45mode->setChecked(true);
    on_dhcp_clicked();
    on_rj45mode_clicked();
}

Rj45IotProgram::~Rj45IotProgram() {
    delete ui;
}

// 获取可用的串口号
void Rj45IotProgram::GetComList () {
    QComboBox *c = ui->com;
    c->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        c->addItem(info.portName() + " " + info.description());
    }
}

void Rj45IotProgram::on_refresh_clicked () {
    GetComList();
}

void Rj45IotProgram::TimerOutEvent () {
    if (btnStatus == 1) {
        ui->tips->setText("设备响应超时");
    } else if (btnStatus == 2) {
        ui->tips->setText("数据接收超时");
    }
    CloseSerial();
}

void Rj45IotProgram::SerialErrorEvent () {
    ui->tips->setText("串口错误");
    CloseSerial();
    GetComList();
}

int Rj45IotProgram::GetBtnStatus () {
    return btnStatus;
}


void Rj45IotProgram::ReadSerialData () {
    QByteArray arr = serial.readAll();
    int len = arr.length();
    char *c = arr.data();
    memcpy(serialReadBuff+bufflen, c, len);
    bufflen += len;
    if (btnStatus == 1) {
        const char set_success[] = {0x73, 0x65 ,0x74 ,0x20 ,0x6F,0x6B, 0x31,0x35}; // set ok
        if (bufflen == sizeof(set_success)) {
            if (!memcmp(serialReadBuff, set_success, sizeof(set_success))) {
                ui->tips->setText("设置成功");
            } else {
                ui->tips->setText("设置失败");
            }
            CloseSerial();
            return;
        }
    } else if (btnStatus == 2) {
        ushort crc = 0xffff;
        crc = COMMON::crc_calc(crc, (uchar*)serialReadBuff, bufflen);
        if (!crc) {
            bufflen -= 2;
            serialReadBuff[bufflen] = '\0';
            yyjson_doc *doc = yyjson_read((char*)serialReadBuff, bufflen, 0);
            if (doc != NULL) {
                yyjson_val *json = yyjson_doc_get_root(doc);
                HandleSerialData(json);
                yyjson_doc_free(doc);
                CloseSerial();
                return;
            }
        }
    }
    timer.stop();
    timer.start(5000);
}

int Rj45IotProgram::OpenSerial (char *data, qint64 len) {
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
    timer.start(5000);
    serial.write(data, len);
    return 0;
}

void Rj45IotProgram::CloseSerial () {
    disconnect(&timer, 0, 0, 0);
    timer.stop();
    disconnect(&serial, 0, 0, 0);
    serial.close();
    bufflen = 0;
    btnStatus = 0;
}

void Rj45IotProgram::on_setmode_clicked() {    
    if (btnStatus) {
        return;
    }
    ui->tips->setText("");
    // 下面的不同项目可能不同，另外，0.5s后没有收到设备返回就会提示失败。
    QByteArray cmqtturl = ui->mqtturl->text().toUtf8();
    int mqtturllen = cmqtturl.length();
    if (!mqtturllen) {
        ui->tips->setText("MQTT URL为空");
        return;
    }
    const char *mqtturl = cmqtturl.data();
    char protocol[16] = {0};
    char host[256] = {0};
    ushort port = 0;
    if (COMMON::urldecode(mqtturl, mqtturllen, protocol, host, &port, NULL) || memcmp(protocol, "tcp", 3) || host[0] == 0 || port == 0) {
        qDebug("protocol:%s, host:%s, port:%d\n", protocol, host, port);
        ui->tips->setText("mqtturl格式错误");
        return;
    }
    QByteArray cmqttuser = ui->mqttuser->text().toUtf8();
    if (!cmqttuser.length()) {
        ui->tips->setText("MQTT USER为空");
        return;
    }
    QByteArray cmqttpass = ui->mqttpass->text().toUtf8();
    if (!cmqttpass.length()) {
        ui->tips->setText("MQTT PASS为空");
        return;
    }
    QByteArray cuser = ui->user->text().toUtf8();
    const char *user = cuser.data();
    if (!COMMON::CheckValidTelephone(user)) {
        ui->tips->setText("拥有者账号格式不正确");
        return;
    }
    QByteArray csntp = ui->sntp->text().toUtf8();
    if (!csntp.length()) {
        ui->tips->setText("sntp服务器地址为空");
        return;
    }
    char buff[1024];
    int len = sprintf(buff, "act=SetMode&Host=%s&Port=%u&UserName=%s&PassWord=%s&User=%s", host, port, cmqttuser.data(), cmqttpass.data(), user);
    if (ui->staticip->isChecked()) {
        QByteArray cip = ui->ip->text().toUtf8();
        const char *ip = cip.data();
        if (!COMMON::CheckValidIp(ip)) {
            ui->tips->setText("ip格式不正确");
            return;
        }
        QByteArray cnetmask = ui->netmask->text().toUtf8();
        const char *netmask = cnetmask.data();
        if (!COMMON::CheckValidIp(netmask)) {
            ui->tips->setText("netmask格式不正确");
            return;
        }
        QByteArray cgateway = ui->gateway->text().toUtf8();
        const char *gateway = cgateway.data();
        if (!COMMON::CheckValidIp(gateway)) {
            ui->tips->setText("gateway格式不正确");
            return;
        }
        QByteArray cdns = ui->dns->text().toUtf8();
        const char *dns = cdns.data();
        if (!COMMON::CheckValidIp(dns)) {
            ui->tips->setText("dns格式不正确");
            return;
        }
        len += sprintf(buff+len, "&Ip=%s&NetMask=%s&GateWay=%s&Dns=%s", ip, netmask, gateway, dns);
    }
    if (ui->wifimode->isChecked()) {
        QByteArray cssid = ui->ssid->text().toUtf8();
        if (!cssid.length()) {
            ui->tips->setText("wifi ssid为空");
            return;
        }
        len += sprintf(buff+len, "&Ssid=%s", cssid.data());
        if (ui->security->isChecked()) {
            QByteArray cwifipass = ui->wifipass->text().toUtf8();
            if (cwifipass.length() < 8) {
                ui->tips->setText("wifi密码长度不够，密码至少8位");
                return;
            }
            len += sprintf(buff+len, "&WifiPass=%s", cwifipass.data());
        }
    }
    ushort crc = 0xffff;
    crc = COMMON::crc_calc(crc, (uchar*)buff, len);
    buff[len] = crc;
    buff[len+1] = crc>>8;
    len += 2;
    if (OpenSerial(buff, len)) {
        ui->tips->setText("串口打开失败");
        return;
    }
    btnStatus = 1;
    ui->tips->setText("数据发送成功");
}

void Rj45IotProgram::on_getmode_clicked() {    
    if (btnStatus) {
        return;
    }
    ui->tips->setText("");
    // 下面的不同项目可能不同，另外，5s后没有收到设备返回就会提示失败。
    char buff[32];
    const char cmd[] = "act=GetMode";
    int len = sizeof(cmd)-1;
    memcpy(buff, cmd, len);
    ushort crc = 0xffff;
    crc = COMMON::crc_calc(crc, (uchar*)buff, len);
    buff[len] = crc;
    buff[len+1] = crc>>8;
    len += 2;
    if (OpenSerial(buff, len)) {
        ui->tips->setText("串口打开失败");
        return;
    }
    btnStatus = 2;
    ui->tips->setText("数据发送成功");
}

void Rj45IotProgram::HandleSerialData (yyjson_val *json) {
    yyjson_val *mqtthost = yyjson_obj_get(json, "Host");
    yyjson_val *mqttport = yyjson_obj_get(json, "Port");
    if (mqtthost && mqttport) {
        char buff[256];
        sprintf(buff, "tcp://%s:%lu", yyjson_get_str(mqtthost), yyjson_get_uint(mqtthost));
        ui->mqtturl->setText(buff);
    }
    yyjson_val *mqttuser = yyjson_obj_get(json, "UserName");
    if (mqttuser) {
        ui->mqttuser->setText(yyjson_get_str(mqttuser));
    }
    yyjson_val *mqttpass = yyjson_obj_get(json, "PassWord");
    if (mqttpass) {
        ui->mqttpass->setText(yyjson_get_str(mqttpass));
    }
    yyjson_val *user = yyjson_obj_get(json, "User");
    if (user) {
        ui->user->setText(yyjson_get_str(user));
    }
    yyjson_val *ip = yyjson_obj_get(json, "Ip");
    yyjson_val *netmask = yyjson_obj_get(json, "NetMask");
    yyjson_val *gateway = yyjson_obj_get(json, "GateWay");
    yyjson_val *dns = yyjson_obj_get(json, "Dns");
    if (ip && netmask && gateway && dns) {
        ui->staticip->setChecked(true);
        on_staticip_clicked();
        ui->ip->setText(yyjson_get_str(ip));
        ui->netmask->setText(yyjson_get_str(netmask));
        ui->gateway->setText(yyjson_get_str(gateway));
        ui->dns->setText(yyjson_get_str(dns));
    } else {
        ui->dhcp->setChecked(true);
        on_dhcp_clicked();
    }
    yyjson_val *ssid = yyjson_obj_get(json, "Ssid");
    if (ssid) {
        ui->wifimode->setChecked(true);
        on_wifimode_clicked();
        ui->ssid->setText(yyjson_get_str(ssid));
        yyjson_val *wifipass = yyjson_obj_get(json, "WifiPass");
        if (wifipass) {
            ui->security->setChecked(true);
            ui->wifipass->setText(yyjson_get_str(wifipass));
        } else {
            ui->security->setChecked(false);
            on_security_clicked();
        }
    } else {
        ui->rj45mode->setChecked(true);
        on_rj45mode_clicked();
    }
    yyjson_val *serialnumber = yyjson_obj_get(json, "Sn");
    if (serialnumber) {
        char buff[128];
        int len = sprintf(buff, "设备SN:%s", yyjson_get_str(serialnumber));
        yyjson_val *type = yyjson_obj_get(json, "Type");
        if (type) {
            sprintf(buff+len, "\r\n设备型号:%s", yyjson_get_str(type));
        }
        ui->tips->setText(buff);
    }
}

void Rj45IotProgram::on_dhcp_clicked() {
    ui->ip->setEnabled(false);
    ui->netmask->setEnabled(false);
    ui->gateway->setEnabled(false);
    ui->dns->setEnabled(false);
}

void Rj45IotProgram::on_staticip_clicked() {
    ui->ip->setEnabled(true);
    ui->netmask->setEnabled(true);
    ui->gateway->setEnabled(true);
    ui->dns->setEnabled(true);
}

void Rj45IotProgram::on_rj45mode_clicked() {
    ui->ssid->setEnabled(false);
    ui->wifipass->setEnabled(false);
    ui->security->setEnabled(false);
}

void Rj45IotProgram::on_wifimode_clicked() {
    ui->ssid->setEnabled(true);
    ui->security->setEnabled(true);
    on_security_clicked();
}

void Rj45IotProgram::on_security_clicked() {
    if (ui->security->isChecked()) {
        ui->wifipass->setEnabled(true);
    } else {
        ui->wifipass->setEnabled(false);
    }
}
