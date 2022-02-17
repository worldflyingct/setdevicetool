#include "isp485.h"
#include "ui_isp485.h"
// 公共函数库
#include "common/common.h"

Isp485::Isp485(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Isp485) {
    ui->setupUi(this);
    groupRadio = new QButtonGroup(this);
    groupRadio->addButton(ui->mqttradio, 0);
    groupRadio->addButton(ui->otherradio, 1);
    ui->mqttradio->setChecked(true);
    connect(ui->mqttradio, SIGNAL(clicked(bool)), this, SLOT(ModeChanged()));
    connect(ui->otherradio, SIGNAL(clicked(bool)), this, SLOT(ModeChanged()));
    GetComList();
}

Isp485::~Isp485() {
    disconnect(ui->mqttradio, 0, 0, 0);
    disconnect(ui->otherradio, 0, 0, 0);
    delete groupRadio;
    delete ui;
}

// 获取可用的串口号
void Isp485::GetComList () {
    QComboBox *c = ui->com;
    for (int a = c->count() ; a > 0 ; a--) {
        c->removeItem(a-1);
    }
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        c->addItem(info.portName() + " " + info.description());
    }
}

void Isp485::on_refresh_clicked() {
    GetComList();
}

void Isp485::TimerOutEvent() {
    if (btnStatus == 1) {
        ui->tips->setText("设备响应超时");
    } else if (btnStatus == 2) {
        ui->tips->setText("数据接收超时");
    }
    CloseSerial();
    btnStatus = 0;
}

int Isp485::GetBtnStatus () {
    return btnStatus;
}

void Isp485::ReadSerialData() {
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
            btnStatus = 0;
            return;
        }
    } else if (btnStatus == 2) {
        unsigned short crc = 0xffff;
        crc = crc_calc(crc, (unsigned char*)serialReadBuff, bufflen);
        if (crc == 0x00) {
            bufflen -= 2;
            serialReadBuff[bufflen] = '\0';
            cJSON *json = cJSON_Parse((char*)serialReadBuff);
            if (json != NULL) {
                HandleSerialData(json);
                cJSON_Delete(json);
                CloseSerial();
                btnStatus = 0;
                return;
            }
        }
    }
    timer.stop();
    timer.start(5000);
}

void Isp485::ModeChanged() {
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

int Isp485::OpenSerial(char *data, qint64 len) {
    char comname[12];
    sscanf(ui->com->currentText().toStdString().c_str(), "%s ", comname);
    serial.setPortName(comname);
    serial.setBaudRate(QSerialPort::Baud115200);
    serial.setParity(QSerialPort::NoParity);
    serial.setDataBits(QSerialPort::Data8);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::NoFlowControl);
    if (!serial.open(QIODevice::ReadWrite)) {
        return -1;
    }
    connect(&timer, SIGNAL(timeout()), this, SLOT(TimerOutEvent()));
    connect(&serial, SIGNAL(readyRead()), this, SLOT(ReadSerialData()));
    timer.start(5000);
    serial.write(data, len);
    return 0;
}

void Isp485::CloseSerial() {
    disconnect(&timer, 0, 0, 0);
    timer.stop();
    disconnect(&serial, 0, 0, 0);
    serial.close();
    bufflen = 0;
}

void Isp485::on_setconfig_clicked() {
    if (btnStatus) {
        return;
    }
    ui->tips->setText("");
    // 下面的不同项目可能不同，另外，0.5s后没有收到设备返回就会提示失败。
    int radio = groupRadio->checkedId();
    if (radio == 0) { // mqtt模式
        std::string curl = ui->url->text().toStdString();
        if (curl.length() == 0) {
            ui->tips->setText("URL为空");
            return;
        }
        const char *url = curl.c_str();
        if (memcmp(url, "tcp://", 6)) {
            ui->tips->setText("MQTT协议不正确");
            return;
        }
        std::string cuser = ui->user->text().toStdString();
        if (cuser.length() == 0) {
            ui->tips->setText("USER为空");
            return;
        }
        std::string cpass = ui->pass->text().toStdString();
        if (cpass.length() == 0) {
            ui->tips->setText("PASS为空");
            return;
        }
        std::string csendtopic = ui->sendtopic->text().toStdString();
        if (csendtopic.length() == 0) {
            ui->tips->setText("发送topic为空");
            return;
        }
        std::string creceivetopic = ui->receivetopic->text().toStdString();
        if (creceivetopic.length() == 0) {
            ui->tips->setText("接收topic为空");
            return;
        }
        const char *user = cuser.c_str();
        const char *pass = cpass.c_str();
        const char *sendtopic = csendtopic.c_str();
        const char *receivetopic = creceivetopic.c_str();
        bool crccheck = ui->crccheck->isChecked();
        std::string cbaudrate = ui->baudrate->currentText().toStdString();
        const char *baudrate = cbaudrate.c_str();
        std::string cdatabits = ui->databits->currentText().toStdString();
        const char *databits = cdatabits.c_str();
        std::string cparitybit = ui->paritybit->currentText().toStdString();
        const char *paritybit = cparitybit.c_str();
        std::string cstopbits = ui->stopbits->currentText().toStdString();
        const char *stopbits = cstopbits.c_str();
        char buff[1024];
        int len = sprintf(buff, "act=SetMode&Mode=0&Url=%s&UserName=%s&PassWord=%s&SendTopic=%s&ReceiveTopic=%s&Crc=%u&BaudRate=%s&DataBits=%s&ParityBit=%s&StopBits=%s",
                                    url, user, pass, sendtopic, receivetopic, crccheck, baudrate, databits, paritybit, stopbits);
        unsigned short crc = 0xffff;
        crc = crc_calc(crc, (unsigned char*)buff, len);
        buff[len] = crc;
        buff[len+1] = crc>>8;
        len += 2;
        btnStatus = 1;
        if (OpenSerial(buff, len)) {
            ui->tips->setText("串口打开失败");
            return;
        }
        ui->tips->setText("数据发送成功");
    } else if (radio == 1) { // tcp或是udp模式
        std::string curl = ui->url->text().toStdString();
        int len = curl.length();
        if (len == 0) {
            ui->tips->setText("URL为空");
            return;
        }
        char url[len+1];
        memcpy(url, curl.c_str(), len);
        url[len] = '\0';
        char *protocol;
        char *host;
        unsigned short port;
        int res = urldecode (url, len+1, &protocol, &host, &port, NULL);
        if (res != 0) {
            ui->tips->setText("url格式错误");
            return;
        }
        unsigned short mode;
        if (!memcmp(protocol, "tcp", 4)) {
            mode = 1;
        } else if (!memcmp(protocol, "udp", 4)) {
            mode = 2;
        } else {
           ui->tips->setText("url格式错误");
           free(protocol);
           free(host);
           return;
        }
        bool crccheck = ui->crccheck->isChecked();
        std::string cbaudrate = ui->baudrate->currentText().toStdString();
        const char *baudrate = cbaudrate.c_str();
        std::string cdatabits = ui->databits->currentText().toStdString();
        const char *databits = cdatabits.c_str();
        std::string cparitybit = ui->paritybit->currentText().toStdString();
        const char *paritybit = cparitybit.c_str();
        std::string cstopbits = ui->stopbits->currentText().toStdString();
        const char *stopbits = cstopbits.c_str();
        char buff[1024];
        len = sprintf(buff, "act=SetMode&Mode=%u&Host=%s&Port=%u&Crc=%u&BaudRate=%s&DataBits=%s&ParityBit=%s&StopBits=%s",
                                    mode, host, port, crccheck, baudrate, databits, paritybit, stopbits);
        free(protocol);
        free(host);
        unsigned short crc = 0xffff;
        crc = crc_calc(crc, (unsigned char*)buff, len);
        buff[len] = crc;
        buff[len+1] = crc>>8;
        len += 2;
        btnStatus = 1;
        if (OpenSerial(buff, len)) {
            ui->tips->setText("串口打开失败");
            btnStatus = 0;
            return;
        }
        ui->tips->setText("数据发送成功");
    }
}

void Isp485::on_getconfig_clicked() {
    if (btnStatus) {
        return;
    }
    ui->tips->setText("");
    // 下面的不同项目可能不同，另外，0.5s后没有收到设备返回就会提示失败。
    char buff[32];
    const char cmd[] = "act=GetMode";
    int len = sizeof(cmd)-1;
    memcpy(buff, cmd, len);
    unsigned short crc = 0xffff;
    crc = crc_calc(crc, (unsigned char*)buff, len);
    buff[len] = crc;
    buff[len+1] = crc>>8;
    len += 2;
    btnStatus = 2;
    if (OpenSerial(buff, len)) {
        ui->tips->setText("串口打开失败");
        btnStatus = 0;
        return;
    }
    ui->tips->setText("数据发送成功");
}

void Isp485::HandleSerialData(cJSON *json) {
    cJSON *mode = cJSON_GetObjectItem(json, "Mode");
    if (mode) {
        if (mode->valueint == 0) { // mqtt
            ui->mqttradio->setChecked(true);
            ModeChanged();
            cJSON *url = cJSON_GetObjectItem(json, "Url");
            if (url) {
                ui->url->setText(url->valuestring);
            }
            cJSON *user = cJSON_GetObjectItem(json, "UserName");
            if (user) {
                ui->user->setText(user->valuestring);
            }
            cJSON *pass = cJSON_GetObjectItem(json, "PassWord");
            if (pass) {
                ui->pass->setText(pass->valuestring);
            }
            cJSON *sendtopic = cJSON_GetObjectItem(json, "SendTopic");
            if (sendtopic) {
                ui->sendtopic->setText(sendtopic->valuestring);
            }
            cJSON *receivetopic = cJSON_GetObjectItem(json, "ReceiveTopic");
            if (receivetopic) {
                ui->receivetopic->setText(receivetopic->valuestring);
            }
        } else { // tcp或udp
            ui->otherradio->setChecked(true);
            ModeChanged();
            cJSON *host = cJSON_GetObjectItem(json, "Host");
            cJSON *port = cJSON_GetObjectItem(json, "Port");
            if (host && port) {
                char url[256];
                if (mode->valueint == 1) { // tcp
                    sprintf(url, "tcp://%s:%u", host->valuestring, port->valueint);
                } else if (mode->valueint == 2) { // udp
                    sprintf(url, "udp://%s:%u", host->valuestring, port->valueint);
                }
                ui->url->setText(url);
            }
        }
    }
    cJSON *crccheck = cJSON_GetObjectItem(json, "Crc");
    if (crccheck) {
        ui->crccheck->setChecked(crccheck->valueint);
    }
    cJSON *baudrate = cJSON_GetObjectItem(json, "BaudRate");
    char str[16];
    if (baudrate) {
        sprintf(str, "%u", baudrate->valueint);
        ui->baudrate->setCurrentText(str);
    }
    cJSON *databits = cJSON_GetObjectItem(json, "DataBits");
    if (databits) {
        sprintf(str, "%u", databits->valueint);
        ui->databits->setCurrentText(str);
    }
    cJSON *paritybit = cJSON_GetObjectItem(json, "ParityBit");
    if (paritybit) {
        ui->paritybit->setCurrentText(paritybit->valuestring);
    }
    cJSON *stopbits = cJSON_GetObjectItem(json, "StopBits");
    if (stopbits) {
        sprintf(str, "%.1f", stopbits->valuedouble);
        ui->stopbits->setCurrentText(str);
    }
    const char tip[] = "设备SN:";
    unsigned short tiplen = sizeof(tip);
    char buff[64];
    memcpy(buff, tip, tiplen);
    cJSON *serialnumber = cJSON_GetObjectItem(json, "Sn");
    if (serialnumber) {
        strcpy(buff + tiplen - 1, serialnumber->valuestring);
        ui->tips->setText(buff);
    }
}
