#include "ispcollector.h"
#include "ui_ispcollector.h"
// 公共函数库
#include "common/common.h"

IspCollector::IspCollector (QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IspCollector) {
    ui->setupUi(this);
    GetComList();
}

IspCollector::~IspCollector () {
    delete ui;
}

// 获取可用的串口号
void IspCollector::GetComList () {
    QComboBox *c = ui->com;
    c->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        c->addItem(info.portName() + " " + info.description());
    }
}

void IspCollector::on_refresh_clicked () {
    GetComList();
}

void IspCollector::TimerOutEvent () {
    if (btnStatus == 1) {
        ui->tips->setText("设备响应超时");
    } else if (btnStatus == 2) {
        ui->tips->setText("数据接收超时");
    }
    CloseSerial();
}

void IspCollector::SerialErrorEvent () {
    ui->tips->setText("串口错误");
    CloseSerial();
    GetComList();
}

int IspCollector::GetBtnStatus () {
    return btnStatus;
}

void IspCollector::ReadSerialData () {
    QByteArray arr = serial.readAll();
    int len = arr.length();
    char *c = arr.data();
    memcpy(serialReadBuff+bufflen, c, len);
    bufflen += len;
    serialReadBuff[bufflen] = '\0';
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

int IspCollector::OpenSerial (char *data, qint64 len) {
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

void IspCollector::CloseSerial () {
    disconnect(&timer, 0, 0, 0);
    timer.stop();
    disconnect(&serial, 0, 0, 0);
    serial.close();
    bufflen = 0;
    btnStatus = 0;
}

void IspCollector::on_setconfig_clicked () {
    if (btnStatus) {
        return;
    }
    QByteArray cserverurl = ui->serverurl->text().toUtf8();
    int len = cserverurl.length();
    if (!len) {
        ui->tips->setText("服务器地址为空");
        return;
    }
    char url[len+1];
    memcpy(url, cserverurl.data(), len);
    url[len] = '\0';
    char protocol[16];
    char host[256];
    ushort port;
    char path[256];
    int res = COMMON::urldecode(url, len, protocol, host, &port, path);
    if (res != 0) {
        ui->tips->setText("url格式错误");
        return;
    }
    ushort mode;
    if (!memcmp(protocol, "coap", 4)) {
        mode = 0;
    } else if (!memcmp(protocol, "udp", 3)) {
        mode = 1;
    } else {
       ui->tips->setText("url格式错误");
       return;
    }
    int day = ui->day->value();
    int hour = ui->hour->value();
    int minute = ui->minute->value();
    int second = ui->second->value();
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

void IspCollector::on_getconfig_clicked () {
    if (btnStatus) {
        return;
    }
    ui->tips->setText("");
    // 下面的不同项目可能不同，另外，5s后没有收到设备返回就会提示失败。
    char buff[32];
    const char cmd[] = "act=GetConfig";
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

void IspCollector::HandleSerialData (yyjson_val *json) {
    yyjson_val *mode = yyjson_obj_get(json, "Mode");
    yyjson_val *host = yyjson_obj_get(json, "Host");
    yyjson_val *port = yyjson_obj_get(json, "Port");
    yyjson_val *path = yyjson_obj_get(json, "Path");
    if (mode && host && port && path) {
        char url[256];
        int m = yyjson_get_int(mode);
        if (m == 0) { // coap
            sprintf(url, "coap://%s:%u/%s", yyjson_get_str(host), yyjson_get_int(port), yyjson_get_str(path));
        } else if (m == 1) { // udp
            sprintf(url, "udp://%s:%u", yyjson_get_str(host), yyjson_get_int(port));
        }
        ui->serverurl->setText(url);
    }
    yyjson_val *secondobj = yyjson_obj_get(json, "Second");
    if (secondobj) {
        int second = yyjson_get_int(secondobj);
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
