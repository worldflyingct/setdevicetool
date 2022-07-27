#include "ispiotprogram.h"
#include "ui_ispiotprogram.h"
// 公共函数库
#include "common/common.h"

IspIotProgram::IspIotProgram (QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IspIotProgram) {
    ui->setupUi(this);
    GetComList();
}

IspIotProgram::~IspIotProgram () {
    delete ui;
}

// 获取可用的串口号
void IspIotProgram::GetComList () {
    QComboBox *c = ui->com;
    c->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        c->addItem(info.portName() + " " + info.description());
    }
}

void IspIotProgram::on_refresh_clicked () {
    GetComList();
}

void IspIotProgram::TimerOutEvent () {
    if (btnStatus == 1) {
        ui->tips->setText("设备响应超时");
    } else if (btnStatus == 2) {
        ui->tips->setText("数据接收超时");
    }
    CloseSerial();
}

void IspIotProgram::SerialErrorEvent () {
    ui->tips->setText("串口错误");
    CloseSerial();
    GetComList();
}

int IspIotProgram::GetBtnStatus () {
    return btnStatus;
}

void IspIotProgram::ReadSerialData () {
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
        unsigned short crc = 0xffff;
        crc = COMMON::crc_calc(crc, (unsigned char*)serialReadBuff, bufflen);
        if (crc == 0x00) {
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

int IspIotProgram::OpenSerial (char *data, qint64 len) {
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
    serial.clear();
    connect(&timer, SIGNAL(timeout()), this, SLOT(TimerOutEvent()));
    connect(&serial, SIGNAL(readyRead()), this, SLOT(ReadSerialData()));
    connect(&serial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(SerialErrorEvent()));
    timer.start(5000);
    serial.write(data, len);
    return 0;
}

void IspIotProgram::CloseSerial () {
    disconnect(&timer, 0, 0, 0);
    timer.stop();
    disconnect(&serial, 0, 0, 0);
    serial.close();
    bufflen = 0;
    btnStatus = 0;
}

void IspIotProgram::on_setmode_clicked () {
    if (btnStatus) {
        return;
    }
    ui->tips->setText("");
    // 下面的不同项目可能不同，另外，0.5s后没有收到设备返回就会提示失败。
    std::string cmqtturl = ui->mqtturl->text().toStdString();
    if (cmqtturl.length() == 0) {
        ui->tips->setText("MQTT URL为空");
        return;
    }
    const char *mqtturl = cmqtturl.c_str();
    if (memcmp(mqtturl, "tcp://", 6)) {
        ui->tips->setText("MQTT协议不正确");
        return;
    }
    std::string cmqttuser = ui->mqttuser->text().toStdString();
    if (!cmqttuser.length()) {
        ui->tips->setText("MQTT USER为空");
        return;
    }
    std::string cmqttpass = ui->mqttpass->text().toStdString();
    if (!cmqttpass.length()) {
        ui->tips->setText("MQTT PASS为空");
        return;
    }
    std::string cuser = ui->user->text().toStdString();
    if (cuser.length() != 11) {
        ui->tips->setText("拥有者账号为空");
        return;
    }
    char buff[1024];
    const char *mqttuser = cmqttuser.c_str();
    const char *mqttpass = cmqttpass.c_str();
    const char *user = cuser.c_str();
    int len = sprintf(buff, "act=SetMode&Url=%s&UserName=%s&PassWord=%s&User=%s", mqtturl, mqttuser, mqttpass, user);
    unsigned short crc = 0xffff;
    crc = COMMON::crc_calc(crc, (unsigned char*)buff, len);
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

void IspIotProgram::on_getmode_clicked () {
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
    crc = COMMON::crc_calc(crc, (unsigned char*)buff, len);
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

void IspIotProgram::HandleSerialData (yyjson_val *json) {
    yyjson_val *mqtturl = yyjson_obj_get(json, "Url");
    if (mqtturl) {
        ui->mqtturl->setText(yyjson_get_str(mqtturl));
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
    const char tip[] = "设备SN:";
    unsigned short tiplen = sizeof(tip);
    char buff[64];
    memcpy(buff, tip, tiplen);
    yyjson_val *serialnumber = yyjson_obj_get(json, "Sn");
    if (serialnumber) {
        strcpy(buff + tiplen - 1, yyjson_get_str(serialnumber));
        ui->tips->setText(buff);
    }
}
