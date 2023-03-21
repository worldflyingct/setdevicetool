#include "smartbuilding22.h"
#include "ui_smartbuilding22.h"

#define ISP_SYNC                    0x7f
#define ISP_SYNC_TWO                0x8f
#define ISP_SYNC_THREE              0x8e
#define ISP_ERASE                   0x0c
#define ISP_ERASE_ACK               0x0d
#define ISP_WRITE                   0x02
#define ISP_WRITE_ACK               0x03
#define ISP_READ                    0x08
#define ISP_READ_ACK                0x09
#define ISP_CHANGE_BAUDRATE         0x12
#define ISP_CHANGE_BAUDRATE_ACK     0x13

Smartbuilding22::Smartbuilding22 (QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Smartbuilding22) {
    ui->setupUi(this);
    GetComList();
}

Smartbuilding22::~Smartbuilding22 () {
    delete ui;
}

// 获取可用的串口号
void Smartbuilding22::GetComList () {
    QComboBox *c = ui->com;
    c->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        c->addItem(info.portName() + " " + info.description());
    }
}

void Smartbuilding22::on_refresh_clicked () {
    GetComList();
}

void Smartbuilding22::TimerOutEvent () {
    if (chipstep == ISP_SYNC || chipstep == ISP_SYNC_TWO) { // sync
        retrytime++;
        if (retrytime < 250) {
            char buff[64];
            sprintf(buff, "等待设备连接...%u", retrytime);
            ui->tips->setText(buff);
            return;
        }
        ui->tips->setText("串口同步失败");
    } else {
        ui->tips->setText("设备响应超时");
    }
    CloseSerial();
}

void Smartbuilding22::SerialErrorEvent () {
    ui->tips->setText("串口错误");
    CloseSerial();
    GetComList();
}

int Smartbuilding22::GetBtnStatus () {
    return btnStatus;
}

void Smartbuilding22::ReadSerialData () {
    QByteArray arr = serial.readAll();
    int len = arr.length();
    char *c = arr.data();
    memcpy(serialReadBuff+bufflen, c, len);
    bufflen += len;
    if (chipstep == ISP_SYNC) {
        if (bufflen < 8) {
            return;
        }
        int step;
        for (step = 0 ; bufflen - step >= 8 ; step++) {
            if (!memcmp(serialReadBuff+step, "TurMass.", 8)) {
                break;
            }
        }
        if (bufflen - step < 8) {
            for (int i = 0 ; i < step ; i++) {
                serialReadBuff[i] = serialReadBuff[i+step];
            }
            bufflen -= step;
            return;
        }
        bufflen = 0;
        chipstep = ISP_SYNC_TWO;
        const char buff[] = "TaoLink.";
        serial.write(buff, 8);
    } else if (chipstep == ISP_SYNC_TWO) { // sync2
        if (bufflen < 2) {
            return;
        }
        if (bufflen > 2 || memcmp(serialReadBuff, "ok", 2)) {
            chipstep = ISP_SYNC;
            return;
        }
        bufflen = 0;
        retrytime = 0;
        ui->tips->setText("串口同步成功，开始修改串口波特率");
        chipstep = ISP_SYNC_THREE;
        char buff[7];
        buff[0] = ISP_CHANGE_BAUDRATE;
        buff[1] = 0x0b; // 设置芯片波特率为912600
        memset(buff+2, 0, 5);
        serial.write(buff, 7);
    } else if (chipstep == ISP_SYNC_THREE) { // sync3
        if (serialReadBuff[0] != ISP_CHANGE_BAUDRATE_ACK) {
            retrytime++;
            if (retrytime == 250) {
                ui->tips->setText("修改波特率失败");
                CloseSerial();
                return;
            }
            bufflen = 0;
            char buff[7];
            buff[0] = ISP_CHANGE_BAUDRATE;
            buff[1] = 0x0b; // 设置芯片波特率为912600
            memset(buff+2, 0, 5);
            serial.write(buff, 7);
        } else {
            bufflen = 0;
            retrytime = 0;
            disconnect(&serial, 0, 0, 0);
            serial.close();
            serial.setBaudRate(921600);
            if (!serial.open(QIODevice::ReadWrite)) {
                char message[64];
                sprintf(message, "程序内部错误%d", __LINE__);
                ui->tips->setText(message);
                CloseSerial();
                return;
            }
            serial.clear();
            connect(&serial, SIGNAL(readyRead()), this, SLOT(ReadSerialData()));
            connect(&serial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(SerialErrorEvent()));
            if (btnStatus == 1) {
                ui->tips->setText("串口波特率修改成功，开始设置设备信息");
            } else if (btnStatus == 2) {
                ui->tips->setText("串口波特率修改成功，开始获取设备信息");
            }
            chipstep = ISP_READ;
            offset = 0;
            addr = 0x6a000;
            uint len = 0x300;
            char buff[7];
            buff[0] = ISP_READ;
            buff[4] = addr>>24;
            buff[3] = addr>>16;
            buff[2] = addr>>8;
            buff[1] = addr;
            buff[6] = len >> 8;
            buff[5] = len;
            serial.write(buff, 7);
        }
    } else if (chipstep == ISP_READ) { // ISP_READ
        if (serialReadBuff[0] != ISP_READ_ACK) {
            retrytime++;
            if (retrytime == 250) {
                ui->tips->setText("读取失败");
                CloseSerial();
                return;
            }
            bufflen = 0;
            uint len = 0x6b000 - addr;
            if (len > 0x300) {
                len = 0x300;
            }
            char buff[7];
            buff[0] = ISP_READ;
            buff[4] = addr>>24;
            buff[3] = addr>>16;
            buff[2] = addr>>8;
            buff[1] = addr;
            buff[6] = len >> 8;
            buff[5] = len;
            serial.write(buff, 7);
        } else {
            retrytime = 0;
            uchar address[4];
            address[3] = addr>>24;
            address[2] = addr>>16;
            address[1] = addr>>8;
            address[0] = addr;
            if (memcmp(address, serialReadBuff+1, 4)) {
                ui->tips->setText("读取失败");
                CloseSerial();
                return;
            }
            uint len = 256*serialReadBuff[6] + serialReadBuff[5];
            if (bufflen == len + 7) { // 获取数据完毕
                memcpy(bin+offset, serialReadBuff+7, len);
                offset += len;
                addr += len;
                len = 0x6b000 - addr;
                if (len > 0x300) {
                    len = 0x300;
                }
                if (len == 0) { // 这里将文件写入
                    if (btnStatus == 2) {
                        char buff[64];
                        sprintf(buff, "读取完毕\r\n设备SN:%s", bin);
                        ui->tips->setText(buff);
                        CloseSerial();
                        ui->speed->setValue(bin[33]);
                        sprintf(buff, "%d", bin[34]);
                        ui->power->setCurrentText(buff);
                        sprintf(buff, "%.2f", (bin[36] + ((uint16_t)bin[37]<<8)) / 100.0);
                        ui->centerfrequency->setText(buff);
                        sprintf(buff, "%.2f", (bin[38] + ((uint16_t)bin[39]<<8)) / 100.0);
                        ui->offsetfrequency->setText(buff);
                        ui->index->setValue(bin[35]);
                        return;
                    } else if (btnStatus == 1) {
                        bufflen = 0;
                        bin[33] = ui->speed->value();
                        int power;
                        sscanf(ui->power->currentText().toUtf8().data(), "%d", &power);
                        bin[34] = power;
                        float centerfrequency;
                        sscanf(ui->centerfrequency->text().toUtf8().data(), "%f", &centerfrequency);
                        uint16_t tmp = int(centerfrequency * 100 + 0.5);
                        bin[36] = tmp;
                        bin[37] = tmp >> 8;
                        float offsetfrequency;
                        sscanf(ui->offsetfrequency->text().toUtf8().data(), "%f", &offsetfrequency);
                        tmp = int(offsetfrequency * 100 + 0.5);
                        bin[38] = tmp;
                        bin[39] = tmp >> 8;
                        bin[35] = ui->index->value();
                        chipstep = ISP_ERASE;
                        addr = 0x6a000;
                        char buff[7];
                        buff[0] = ISP_ERASE;
                        buff[4] = addr>>24;
                        buff[3] = addr>>16;
                        buff[2] = addr>>8;
                        buff[1] = addr;
                        buff[6] = 0;
                        buff[5] = 0;
                        serial.write(buff, 7);
                    }
                } else {
                    bufflen = 0;
                    char buff[7];
                    buff[0] = ISP_READ;
                    buff[4] = addr>>24;
                    buff[3] = addr>>16;
                    buff[2] = addr>>8;
                    buff[1] = addr;
                    buff[6] = len >> 8;
                    buff[5] = len;
                    serial.write(buff, 7);
                }
            }
        }
    } else if (chipstep == ISP_ERASE) { // ISP_ERASE
        if (serialReadBuff[0] != ISP_ERASE_ACK) {
            retrytime++;
            if (retrytime == 250) {
                ui->tips->setText("擦除失败");
                CloseSerial();
                return;
            }
            addr = 0x6a000;
            bufflen = 0;
            offset = 0;
            char buff[7];
            buff[0] = ISP_ERASE;
            buff[4] = addr>>24;
            buff[3] = addr>>16;
            buff[2] = addr>>8;
            buff[1] = addr;
            buff[6] = 0;
            buff[5] = 0;
            serial.write(buff, 7);
        } else {
            retrytime = 0;
            bufflen = 0;
            uchar address[4];
            address[3] = addr>>24;
            address[2] = addr>>16;
            address[1] = addr>>8;
            address[0] = addr;
            if (memcmp(address, serialReadBuff+1, 4) || serialReadBuff[5] != 0x00 || serialReadBuff[6] != 0x00) {
                ui->tips->setText("擦除失败");
                CloseSerial();;
                return;
            }
            chipstep = ISP_WRITE;
            offset = 0;
            uint len = 0x6b000 - addr;
            if (len > 0x100) {
                len = 0x100;
            }
            char *serialWriteBuff = (char*)malloc(len+7);
            if (serialWriteBuff == NULL) {
                ui->tips->setText("内存申请失败");
                CloseSerial();
                return;
            }
            serialWriteBuff[0] = ISP_WRITE;
            serialWriteBuff[4] = addr >> 24;
            serialWriteBuff[3] = addr >> 16;
            serialWriteBuff[2] = addr >> 8;
            serialWriteBuff[1] = addr;
            serialWriteBuff[6] = len >> 8;
            serialWriteBuff[5] = len;
            memcpy(serialWriteBuff + 7, bin + offset, len);
            serial.write(serialWriteBuff, len+7);
            free(serialWriteBuff);
        }
    } else if (chipstep == ISP_WRITE) { // ISP_WRITE
        if (serialReadBuff[0] != ISP_WRITE_ACK) {
            retrytime++;
            if (retrytime == 250) {
                ui->tips->setText("写入失败");
                CloseSerial();
                return;
            }
            bufflen = 0;
            uint len = 0x6b000 - addr;
            if (len > 0x100) {
                len = 0x100;
            }
            char *serialWriteBuff = (char*)malloc(len+7);
            if (serialWriteBuff == NULL) {
                ui->tips->setText("内存申请失败");
                CloseSerial();
                return;
            }
            serialWriteBuff[0] = ISP_WRITE;
            serialWriteBuff[4] = addr >> 24;
            serialWriteBuff[3] = addr >> 16;
            serialWriteBuff[2] = addr >> 8;
            serialWriteBuff[1] = addr;
            serialWriteBuff[6] = len >> 8;
            serialWriteBuff[5] = len;
            memcpy(serialWriteBuff + 7, bin + offset, len);
            serial.write(serialWriteBuff, len+7);
            free(serialWriteBuff);
        } else {
            retrytime = 0;
            bufflen = 0;
            uchar address[4];
            address[3] = addr>>24;
            address[2] = addr>>16;
            address[1] = addr>>8;
            address[0] = addr;
            if (memcmp(address, serialReadBuff+1, 4)) {
                ui->tips->setText("写入数据失败");
                CloseSerial();
                return;
            }
            uint len = 256*serialReadBuff[6] + serialReadBuff[5];
            offset += len;
            addr += len;
            len = 0x6b000 - addr;
            if (len > 0x100) {
                len = 0x100;
            }
            if (len == 0) {
                ui->tips->setText("设备数据更新完毕");
                CloseSerial();
                return;
            }
            char *serialWriteBuff = (char*)malloc(len+7);
            if (serialWriteBuff == NULL) {
                ui->tips->setText("内存申请失败");
                CloseSerial();
                return;
            }
            serialWriteBuff[0] = ISP_WRITE;
            serialWriteBuff[4] = addr >> 24;
            serialWriteBuff[3] = addr >> 16;
            serialWriteBuff[2] = addr >> 8;
            serialWriteBuff[1] = addr;
            serialWriteBuff[6] = len >> 8;
            serialWriteBuff[5] = len;
            memcpy(serialWriteBuff + 7, bin + offset, len);
            serial.write(serialWriteBuff, len+7);
            free(serialWriteBuff);
        }
    } else {
        char buff[64];
        sprintf(buff, "chipstep:0x%02x, bufflen:%d, ack:0x%02x", chipstep, bufflen, serialReadBuff[0]);
        ui->tips->setText(buff);
        CloseSerial();
        return;
    }
    timer.stop();
    if (chipstep == ISP_SYNC || chipstep == ISP_SYNC_TWO) {
        timer.start(1000); // 正在同步，所需时间比较短
    } else {
        timer.start(3000); // 非ISP_SYNC命令，非擦除命令，统一等待3s
    }
}

int Smartbuilding22::OpenSerial () {
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

void Smartbuilding22::CloseSerial () {
    disconnect(&timer, 0, 0, 0);
    timer.stop();
    disconnect(&serial, 0, 0, 0);
    serial.close();
    bufflen = 0;
    btnStatus = 0;
}

void Smartbuilding22::on_getconfig_clicked() {
    if (btnStatus) {
        ui->tips->setText("用户终止操作");
        CloseSerial();
        return;
    }
    retrytime = 0;
    chipstep = ISP_SYNC;
    if (OpenSerial()) {
        ui->tips->setText("串口打开失败");
        return;
    }
    btnStatus = 2;
    char buff[64];
    sprintf(buff, "等待设备连接...%u", ++retrytime);
    ui->tips->setText(buff);
}

void Smartbuilding22::on_setconfig_clicked() {
    if (btnStatus) {
        ui->tips->setText("用户终止操作");
        CloseSerial();
        return;
    }
    retrytime = 0;
    chipstep = ISP_SYNC;
    if (OpenSerial()) {
        ui->tips->setText("串口打开失败");
        return;
    }
    btnStatus = 1;
    char buff[64];
    sprintf(buff, "等待设备连接...%u", ++retrytime);
    ui->tips->setText(buff);
}
