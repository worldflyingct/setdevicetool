#include "isplocation.h"
#include "ui_isplocation.h"
// 公共函数库
#include "common/common.h"

IspLocation::IspLocation(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IspLocation) {
    ui->setupUi(this);
    GetComList();
}

IspLocation::~IspLocation() {
    delete ui;
}

// 获取可用的串口号
void IspLocation::GetComList () {
    QComboBox *c = ui->com;
    for (int a = c->count() ; a > 0 ; a--) {
        c->removeItem(a-1);
    }
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        c->addItem(info.portName() + " " + info.description());
    }
}

void IspLocation::on_refresh_clicked() {
    GetComList();
}

void IspLocation::TimerOutEvent() {
    if (btnStatus == 1) {
        ui->tips->setText("设备响应超时");
    } else if (btnStatus == 2) {
        ui->tips->setText("数据接收超时");
    }
    CloseSerial();
    btnStatus = 0;
}

void IspLocation::ReadSerialData() {
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

int IspLocation::OpenSerial(char *data, qint64 len) {
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

void IspLocation::CloseSerial() {
    disconnect(&timer, 0, 0, 0);
    timer.stop();
    disconnect(&serial, 0, 0, 0);
    serial.close();
    bufflen = 0;
}

void IspLocation::on_setconfig_clicked() {
    std::string cserverurl = ui->serverurl->text().toStdString();
    int len = cserverurl.length();
    if (len == 0) {
        ui->tips->setText("服务器地址为空");
        return;
    }
    char url[len+1];
    memcpy(url, cserverurl.c_str(), len);
    url[len] = '\0';
    char *protocol;
    char *host;
    unsigned short port;
    char *path;
    int res = urldecode(url, len+1, &protocol, &host, &port, &path);
    if (res != 0) {
        ui->tips->setText("url格式错误");
        return;
    }
    unsigned short mode;
    if (!memcmp(protocol, "coap", 4)) {
        mode = 0;
    } else if (!memcmp(protocol, "udp", 3)) {
        mode = 1;
    } else {
       ui->tips->setText("url格式错误");
       free(protocol);
       free(host);
       free(path);
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
    char buff[1024];
    if (mode == 0) {
        len = sprintf(buff, "act=SetConfig&Mode=0&Host=%s&Port=%d&Second=%d", host, port, totalsecond);
    } else if (mode == 1){
        len = sprintf(buff, "act=SetConfig&Mode=1&Host=%s&Port=%d&Path=%s&Second=%d", host, port, path, totalsecond);
    }
    free(protocol);
    free(host);
    free(path);
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
}

void IspLocation::on_getconfig_clicked() {
    ui->tips->setText("");
    // 下面的不同项目可能不同，另外，0.5s后没有收到设备返回就会提示失败。
    char buff[32];
    const char cmd[] = "act=GetConfig";
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
        return;
    }
    ui->tips->setText("数据发送成功");
}

void IspLocation::HandleSerialData(cJSON *json) {
    cJSON *mode = cJSON_GetObjectItem(json, "Mode");
    cJSON *host = cJSON_GetObjectItem(json, "Host");
    cJSON *port = cJSON_GetObjectItem(json, "Port");
    cJSON *path = cJSON_GetObjectItem(json, "Path");
    if (mode && host && port && path) {
        char url[256];
        if (mode->valueint == 0) { // coap
            sprintf(url, "coap://%s:%u/%s", host->valuestring, port->valueint, path->valuestring);
        } else if (mode->valueint == 1) { // udp
            sprintf(url, "udp://%s:%u", host->valuestring, port->valueint);
        }
        ui->serverurl->setText(url);
    }
    cJSON *secondobj = cJSON_GetObjectItem(json, "Second");
    if (secondobj) {
        int second = secondobj->valueint;
        int day = second / (24*60*60);
        second = second % (24*60*60);
        int hour = second / 3600;
        second = second % 3600;
        int minute = second / 60;
        second = second % 60;
        ui->day->setValue(day);
        ui->hour->setValue(hour);
        ui->minute->setValue(minute);
        ui->second->setValue(second);
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
