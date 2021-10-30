#include "ispiocontroller.h"
#include "ui_ispiocontroller.h"
// cjson库
#include "common/cjson.h"
// 公共函数库
#include "common/common.h"

IspIoController::IspIoController(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IspIoController)
{
    ui->setupUi(this);
    GetComList();
}

IspIoController::~IspIoController()
{
    delete ui;
}

// 获取可用的串口号
void IspIoController::GetComList ()
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

void IspIoController::on_refresh_clicked()
{
    this->GetComList();
}

void IspIoController::TimerOutEvent()
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

void IspIoController::ReadSerialData()
{
    QByteArray arr = serial.readAll();
    int len = arr.length();
    char *c = arr.data();
    memcpy(serialReadBuff+bufflen, c, len);
    bufflen += len;
    timer.stop();
    timer.start(500);
}

int IspIoController::OpenSerial()
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

void IspIoController::CloseSerial()
{
    disconnect(&timer, 0, 0, 0);
    timer.stop();
    disconnect(&serial, 0, 0, 0);
    serial.close();
}

void IspIoController::on_setmqtt_clicked()
{
    ui->tips->setText("");
    if (OpenSerial()) {
        ui->tips->setText("串口打开失败");
        return;
    }
    ui->tips->setText("串口打开成功");
    // 下面的不同项目可能不同，另外，0.5s后没有收到设备返回就会提示失败。
    btnStatus = 1;
    char buff[1024];
    std::string cmqtturl = ui->mqtturl->text().toStdString();
    if (cmqtturl.length() == 0) {
        ui->tips->setText("MQTT URL为空");
        CloseSerial();
        return;
    }
    const char *mqtturl = cmqtturl.c_str();
    if (memcmp(mqtturl, "tcp://", 6)) {
        ui->tips->setText("MQTT协议不正确");
        CloseSerial();
        return;
    }
    std::string cmqttuser = ui->mqttuser->text().toStdString();
    const char *mqttuser = cmqttuser.c_str();
    if (!cmqttuser.length()) {
        ui->tips->setText("MQTT USER为空");
        CloseSerial();
        return;
    }
    std::string cmqttpass = ui->mqttpass->text().toStdString();
    if (!cmqttpass.length()) {
        ui->tips->setText("MQTT PASS为空");
        CloseSerial();
        return;
    }
    const char *mqttpass = cmqttpass.c_str();
    int len = sprintf(buff, "act=SetMqtt&Url=%s&UserName=%s&PassWord=%s", mqtturl, mqttuser, mqttpass);
    unsigned short crc = 0xffff;
    crc = crc_calc(crc, (unsigned char*)buff, len);
    buff[len] = crc;
    buff[len+1] = crc>>8;
    len += 2;
    serial.write(buff, len);
}

void IspIoController::on_readmqtt_clicked()
{
    ui->tips->setText("");
    if (OpenSerial()) {
        ui->tips->setText("串口打开失败");
        return;
    }
    ui->tips->setText("串口打开成功");
    // 下面的不同项目可能不同，另外，0.5s后没有收到设备返回就会提示失败。
    btnStatus = 2;
    char buff[1024];
    const char cmd[] = "act=GetMqtt";
    int len = sizeof(cmd)-1;
    memcpy(buff, cmd, len);
    unsigned short crc = 0xffff;
    crc = crc_calc(crc, (unsigned char*)buff, len);
    buff[len] = crc;
    buff[len+1] = crc>>8;
    len += 2;
    serial.write(buff, len);
}

void IspIoController::HandleSerialData()
{
    if (btnStatus == 1) {
        const char set_success[] = {0x73, 0x65 ,0x74 ,0x20 ,0x6F,0x6B, 0x31,0x35};
        if (bufflen != sizeof(set_success) || memcmp(serialReadBuff, set_success, sizeof(set_success))) {
            ui->tips->setText("串口设置失败");
        } else {
            ui->tips->setText("串口设置成功");
        }
    } else if (btnStatus == 2) {
        unsigned short crc = 0xffff;
        crc = crc_calc(crc, (unsigned char*)serialReadBuff, bufflen);
        if (crc == 0x00) {
            bufflen -= 2;
            serialReadBuff[bufflen] = '\0';
            cJSON *json = cJSON_Parse((char*)serialReadBuff);
            if (json) {
                cJSON *mqtturl = cJSON_GetObjectItem(json, "Url");
                ui->mqtturl->setText(mqtturl->valuestring);
                cJSON *mqttuser = cJSON_GetObjectItem(json, "UserName");
                ui->mqttuser->setText(mqttuser->valuestring);
                cJSON *mqttpass = cJSON_GetObjectItem(json, "PassWord");
                ui->mqttpass->setText(mqttpass->valuestring);
                const char cmd[] = "设备SN:";
                char buff[1024];
                memcpy(buff, cmd, sizeof(cmd));
                cJSON *serialnumber = cJSON_GetObjectItem(json, "Sn");
                strcat(buff, serialnumber->valuestring);
                ui->tips->setText(buff);
                cJSON_Delete(json);
            } else {
                ui->tips->setText("数据解析错误");
            }
        } else {
            ui->tips->setText("crc校验错误");
        }
    }
    btnStatus = 0;
}
