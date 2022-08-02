#include "isp485.h"
#include "ui_isp485.h"
// 公共函数库
#include "common/common.h"

Isp485::Isp485 (QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Isp485) {
    ui->setupUi(this);
    GetComList();
}

Isp485::~Isp485 () {
    delete ui;
}

// 获取可用的串口号
void Isp485::GetComList () {
    QComboBox *c = ui->com;
    c->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        c->addItem(info.portName() + " " + info.description());
    }
}

void Isp485::on_refresh_clicked () {
    GetComList();
}

void Isp485::TimerOutEvent () {
    if (btnStatus == 1) {
        ui->tips->setText("设备响应超时");
    } else if (btnStatus == 2) {
        ui->tips->setText("数据接收超时");
    }
    CloseSerial();
}

void Isp485::SerialErrorEvent () {
    ui->tips->setText("串口错误");
    CloseSerial();
    GetComList();
}

int Isp485::GetBtnStatus () {
    return btnStatus;
}

void Isp485::ReadSerialData () {
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

int Isp485::OpenSerial (char *data, qint64 len) {
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

void Isp485::CloseSerial () {
    disconnect(&timer, 0, 0, 0);
    timer.stop();
    disconnect(&serial, 0, 0, 0);
    serial.close();
    bufflen = 0;
    btnStatus = 0;
}

void Isp485::on_mqttradio_clicked () {
    ui->user->setEnabled(true);
    ui->pass->setEnabled(true);
    ui->sendtopic->setEnabled(true);
    ui->receivetopic->setEnabled(true);
}

void Isp485::on_otherradio_clicked () {
    ui->user->setEnabled(false);
    ui->pass->setEnabled(false);
    ui->sendtopic->setEnabled(false);
    ui->receivetopic->setEnabled(false);
}

void Isp485::on_setconfig_clicked () {
    if (btnStatus) {
        return;
    }
    ui->tips->setText("");
    // 下面的不同项目可能不同，另外，5s后没有收到设备返回就会提示失败。
    if (ui->mqttradio->isChecked()) { // mqtt模式
        QByteArray curl = ui->url->text().toUtf8();
        if (!curl.length()) {
            ui->tips->setText("URL为空");
            return;
        }
        const char *url = curl.data();
        if (memcmp(url, "tcp://", 6)) {
            ui->tips->setText("MQTT协议不正确");
            return;
        }
        QByteArray cuser = ui->user->text().toUtf8();
        if (!cuser.length()) {
            ui->tips->setText("USER为空");
            return;
        }
        QByteArray cpass = ui->pass->text().toUtf8();
        if (!cpass.length()) {
            ui->tips->setText("PASS为空");
            return;
        }
        QByteArray csendtopic = ui->sendtopic->text().toUtf8();
        if (!csendtopic.length()) {
            ui->tips->setText("发送topic为空");
            return;
        }
        QByteArray creceivetopic = ui->receivetopic->text().toUtf8();
        if (!creceivetopic.length()) {
            ui->tips->setText("接收topic为空");
            return;
        }
        const char *user = cuser.data();
        const char *pass = cpass.data();
        const char *sendtopic = csendtopic.data();
        const char *receivetopic = creceivetopic.data();
        bool crccheck = ui->crccheck->isChecked();
        QByteArray cbaudrate = ui->baudrate->currentText().toUtf8();
        const char *baudrate = cbaudrate.data();
        QByteArray cdatabits = ui->databits->currentText().toUtf8();
        const char *databits = cdatabits.data();
        QByteArray cparitybit = ui->paritybit->currentText().toUtf8();
        const char *paritybit = cparitybit.data();
        QByteArray cstopbits = ui->stopbits->currentText().toUtf8();
        const char *stopbits = cstopbits.data();
        char buff[1024];
        int len = sprintf(buff, "act=SetMode&Mode=0&Url=%s&UserName=%s&PassWord=%s&SendTopic=%s&ReceiveTopic=%s&Crc=%u&BaudRate=%s&DataBits=%s&ParityBit=%s&StopBits=%s",
                                    url, user, pass, sendtopic, receivetopic, crccheck, baudrate, databits, paritybit, stopbits);
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
    } else if (ui->otherradio->isChecked()) { // tcp或是udp模式
        QByteArray curl = ui->url->text().toUtf8();
        int len = curl.length();
        if (!len) {
            ui->tips->setText("URL为空");
            return;
        }
        char url[len+1];
        memcpy(url, curl.data(), len);
        url[len] = '\0';
        char protocol[16];
        char host[256];
        ushort port;
        int res = COMMON::urldecode (url, len, protocol, host, &port, NULL);
        if (res != 0) {
            ui->tips->setText("url格式错误");
            return;
        }
        ushort mode;
        if (!memcmp(protocol, "tcp", 4)) {
            mode = 1;
        } else if (!memcmp(protocol, "udp", 4)) {
            mode = 2;
        } else {
           ui->tips->setText("url格式错误");
           return;
        }
        bool crccheck = ui->crccheck->isChecked();
        QByteArray cbaudrate = ui->baudrate->currentText().toUtf8();
        const char *baudrate = cbaudrate.data();
        QByteArray cdatabits = ui->databits->currentText().toUtf8();
        const char *databits = cdatabits.data();
        QByteArray cparitybit = ui->paritybit->currentText().toUtf8();
        const char *paritybit = cparitybit.data();
        QByteArray cstopbits = ui->stopbits->currentText().toUtf8();
        const char *stopbits = cstopbits.data();
        char buff[1024];
        len = sprintf(buff, "act=SetMode&Mode=%u&Host=%s&Port=%u&Crc=%u&BaudRate=%s&DataBits=%s&ParityBit=%s&StopBits=%s",
                                    mode, host, port, crccheck, baudrate, databits, paritybit, stopbits);
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
}

void Isp485::on_getconfig_clicked () {
    if (btnStatus) {
        return;
    }
    ui->tips->setText("");
    // 下面的不同项目可能不同，另外，0.5s后没有收到设备返回就会提示失败。
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

void Isp485::HandleSerialData (yyjson_val *json) {
    yyjson_val *mode = yyjson_obj_get(json, "Mode");
    if (mode) {
        int m = yyjson_get_int(mode);
        if (m == 0) { // mqtt
            ui->mqttradio->setChecked(true);
            on_mqttradio_clicked();
            yyjson_val *url = yyjson_obj_get(json, "Url");
            if (url) {
                ui->url->setText(yyjson_get_str(url));
            }
            yyjson_val *user = yyjson_obj_get(json, "UserName");
            if (user) {
                ui->user->setText(yyjson_get_str(user));
            }
            yyjson_val *pass = yyjson_obj_get(json, "PassWord");
            if (pass) {
                ui->pass->setText(yyjson_get_str(pass));
            }
            yyjson_val *sendtopic = yyjson_obj_get(json, "SendTopic");
            if (sendtopic) {
                ui->sendtopic->setText(yyjson_get_str(sendtopic));
            }
            yyjson_val *receivetopic = yyjson_obj_get(json, "ReceiveTopic");
            if (receivetopic) {
                ui->receivetopic->setText(yyjson_get_str(receivetopic));
            }
        } else { // tcp或udp
            ui->otherradio->setChecked(true);
            on_otherradio_clicked();
            yyjson_val *host = yyjson_obj_get(json, "Host");
            yyjson_val *port = yyjson_obj_get(json, "Port");
            if (host && port) {
                char url[256];
                if (m == 1) { // tcp
                    sprintf(url, "tcp://%s:%u", yyjson_get_str(host), yyjson_get_int(port));
                } else if (m == 2) { // udp
                    sprintf(url, "udp://%s:%u", yyjson_get_str(host), yyjson_get_int(port));
                }
                ui->url->setText(url);
            }
        }
    }
    yyjson_val *crccheck = yyjson_obj_get(json, "Crc");
    if (crccheck) {
        ui->crccheck->setChecked(yyjson_get_int(crccheck));
    }
    yyjson_val *baudrate = yyjson_obj_get(json, "BaudRate");
    char str[16];
    if (baudrate) {
        sprintf(str, "%u", yyjson_get_int(baudrate));
        ui->baudrate->setCurrentText(str);
    }
    yyjson_val *databits = yyjson_obj_get(json, "DataBits");
    if (databits) {
        sprintf(str, "%u", yyjson_get_int(databits));
        ui->databits->setCurrentText(str);
    }
    yyjson_val *paritybit = yyjson_obj_get(json, "ParityBit");
    if (paritybit) {
        ui->paritybit->setCurrentText(yyjson_get_str(paritybit));
    }
    yyjson_val *stopbits = yyjson_obj_get(json, "StopBits");
    if (stopbits) {
        sprintf(str, "%.1f", yyjson_get_real(stopbits));
        ui->stopbits->setCurrentText(str);
    }
    const char tip[] = "设备SN:";
    ushort tiplen = sizeof(tip);
    char buff[64];
    memcpy(buff, tip, tiplen);
    yyjson_val *serialnumber = yyjson_obj_get(json, "Sn");
    if (serialnumber) {
        strcpy(buff + tiplen - 1, yyjson_get_str(serialnumber));
        ui->tips->setText(buff);
    }
}
