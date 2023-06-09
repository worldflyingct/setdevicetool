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

#define ISP_DEVICEMODE              0x20
#define ISP_FREQ                    0x21
#define ISP_TXP                     0x22
#define ISP_MODE                    0x23
#define ISP_SLOTBYTES               0x24
#define ISP_SETDEVICE               0x25
#define ISP_WAITSN                  0x26
#define ISP_WAITSN2                 0x27

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
    serialReadBuff[bufflen] = '\0';
    if (chipstep == ISP_SYNC) {
        if (bufflen < 8) {
            return;
        }
        int i, step;
        for (step = 0 ; bufflen - step >= 8 ; step++) {
            if (!memcmp(serialReadBuff+step, "TurMass.", 8)) {
                break;
            }
        }
        if (bufflen - step < 8) {
            for (i = 0 ; i + step < bufflen ; i++) {
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
        buff[1] = 0x0b; // 设置芯片波特率为921600
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
            buff[1] = 0x0b; // 设置芯片波特率为921600
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
    } else if (chipstep == ISP_DEVICEMODE) {
        if (bufflen < 7) {
            return;
        }
        int i, step;
        for (step = 0 ; bufflen - step >= 7 ; step++) {
            if (!memcmp(serialReadBuff+step, "AT_OK\r\n", 7)) {
                break;
            }
        }
        if (bufflen - step < 7) {
            for (i = 0 ; i + step < bufflen ; i++) {
                serialReadBuff[i] = serialReadBuff[i+step];
            }
            bufflen -= step;
            return;
        }
        bufflen = 0;
        ui->tips->setText("设置通信器为时隙模式主机成功，开始设置通信器通信频点");
        chipstep = ISP_FREQ;
        const char buff[] = "AT+FREQ=475330000:475300000\r\n";
        serial.write(buff, 29);
    } else if (chipstep == ISP_FREQ) {
        if (bufflen < 7) {
            return;
        }
        int i, step;
        for (step = 0 ; bufflen - step >= 7 ; step++) {
            if (!memcmp(serialReadBuff+step, "AT_OK\r\n", 7)) {
                break;
            }
        }
        if (bufflen - step < 7) {
            for (i = 0 ; i + step < bufflen ; i++) {
                serialReadBuff[i] = serialReadBuff[i+step];
            }
            bufflen -= step;
            return;
        }
        bufflen = 0;
        ui->tips->setText("设置通信器通信频点成功，开始设置通信器发送功率");
        chipstep = ISP_TXP;
        const char buff[] = "AT+TXP=15\r\n";
        serial.write(buff, 11);
    } else if (chipstep == ISP_TXP) {
        if (bufflen < 7) {
            return;
        }
        int i, step;
        for (step = 0 ; bufflen - step >= 7 ; step++) {
            if (!memcmp(serialReadBuff+step, "AT_OK\r\n", 7)) {
                break;
            }
        }
        if (bufflen - step < 7) {
            for (i = 0 ; i + step < bufflen ; i++) {
                serialReadBuff[i] = serialReadBuff[i+step];
            }
            bufflen -= step;
            return;
        }
        bufflen = 0;
        ui->tips->setText("设置通信器发送功率成功，开始设置通信器通信速率");
        chipstep = ISP_MODE;
        const char buff[] = "AT+MODE=16:16\r\n";
        serial.write(buff, 15);
    } else if (chipstep == ISP_MODE) {
        if (bufflen < 7) {
            return;
        }
        int i, step;
        for (step = 0 ; bufflen - step >= 7 ; step++) {
            if (!memcmp(serialReadBuff+step, "AT_OK\r\n", 7)) {
                break;
            }
        }
        if (bufflen - step < 7) {
            for (i = 0 ; i + step < bufflen ; i++) {
                serialReadBuff[i] = serialReadBuff[i+step];
            }
            bufflen -= step;
            return;
        }
        bufflen = 0;
        ui->tips->setText("设置通信器通信速率成功，开始设置通信器数据包大小");
        chipstep = ISP_SLOTBYTES;
        const char buff[] = "AT+SLOTBYTES=33\r\n";
        serial.write(buff, 17);
    } else if (chipstep == ISP_SLOTBYTES) {
        if (bufflen < 7) {
            return;
        }
        int i, step;
        for (step = 0 ; bufflen - step >= 7 ; step++) {
            if (!memcmp(serialReadBuff+step, "AT_OK\r\n", 7)) {
                break;
            }
        }
        if (bufflen - step < 7) {
            for (i = 0 ; i + step < bufflen ; i++) {
                serialReadBuff[i] = serialReadBuff[i+step];
            }
            bufflen -= step;
            return;
        }
        bufflen = 0;
        ui->tips->setText("设置通信器数据包大小成功，开始发送设备参数");
        chipstep = ISP_SETDEVICE;
        float centerfrequency;
        sscanf(ui->centerfrequency->text().toUtf8().data(), "%f", &centerfrequency);
        int center = int(centerfrequency * 100 + 0.5);
        float offsetfrequency;
        sscanf(ui->offsetfrequency->text().toUtf8().data(), "%f", &offsetfrequency);
        int offset = int(offsetfrequency * 100 + 0.5);
        int speed = ui->speed->value();
        int index = ui->index->value();
        char buff[64];
        int len = sprintf(buff, "AT+SENDB=0:0083ffffffffffffffffffffffffffffffff%04x%04x%02x%02x\r\n", center, offset, speed, index);
        serial.write(buff, len);
    } else if (chipstep == ISP_SETDEVICE) {
        if (bufflen < 4) {
            return;
        }
        int i, step;
        for (step = 0 ; bufflen - step >= 4 ; step++) {
            if (!memcmp(serialReadBuff+step, "Data", 4)) {
                break;
            }
        }
        int newbufflen = bufflen - step;
        for (i = 0 ; i + step < bufflen ; i++) {
            serialReadBuff[i] = serialReadBuff[i+step];
        }
        bufflen = newbufflen;
        if (newbufflen < 4) {
            return;
        }
        for (step = 0 ; bufflen - step >= 2 ; step++) {
            if (!memcmp(serialReadBuff+step, "\r\n", 2)) {
                break;
            }
        }
        if (bufflen - step < 2) {
            return;
        }
        step = 4;
        while (serialReadBuff[step] == ' ') {
            step++;
        }
        int a = 0;
        char sn[33];
        while (serialReadBuff[step] != ' ' && serialReadBuff[step] != '\r') {
            sn[a] = serialReadBuff[step] <= '9' ? serialReadBuff[step]-'0' : serialReadBuff[step]+10-'A';
            sn[a] *= 16;
            step++;
            sn[a] += serialReadBuff[step] <= '9' ? serialReadBuff[step]-'0' : serialReadBuff[step]+10-'A';
            step++;
            a++;
        }
        sn[a] = '\0';
        char buff[256];
        sprintf(buff, "设置设备参数成功\r\nsn:%s", sn);
        ui->tips->setText(buff);
        CloseSerial();
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
                        CloseSerial();
                        char buff[64];
                        bin[32] = '\0';
                        sprintf(buff, "读取完毕\r\n设备SN:%s", bin);
                        ui->tips->setText(buff);
                        ui->speed->setValue(bin[33]);
                        sprintf(buff, "%d", bin[34]);
                        ui->power->setCurrentText(buff);
                        sprintf(buff, "%.2f", (bin[36] + ((uint16_t)bin[37]<<8)) / 100.0);
                        ui->centerfrequency->setText(buff);
                        sprintf(buff, "%.2f", (bin[38] + ((uint16_t)bin[39]<<8)) / 100.0);
                        ui->offsetfrequency->setText(buff);
                        ui->index->setValue(bin[35]);
                        ui->uploadmode->setChecked(bin[40]); // 0是帧上传，1是立刻上传。
                        uint32_t sleep_num1 = (uint32_t)bin[44] + 256*(uint32_t)bin[45] + 256*256*(uint32_t)bin[46] + 256*256*256*(uint32_t)bin[47];
                        ui->sleep_num1->setValue(sleep_num1);
                        uint32_t sleep_num2 = (uint32_t)bin[48] + 256*(uint32_t)bin[49] + 256*256*(uint32_t)bin[50] + 256*256*256*(uint32_t)bin[51];
                        ui->sleep_num2->setValue(sleep_num2);
                        ui->pr->setValue(bin[52]);
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
                        bin[40] = ui->uploadmode->isChecked();
                        if (writesleepnum) {
                            uint32_t sleep_num1 = ui->sleep_num1->value();
                            bin[44] = sleep_num1;
                            bin[45] = sleep_num1 >> 8;
                            bin[46] = sleep_num1 >> 16;
                            bin[47] = sleep_num1 >> 24;
                            uint32_t sleep_num2 = ui->sleep_num2->value();
                            bin[48] = sleep_num2;
                            bin[49] = sleep_num2 >> 8;
                            bin[50] = sleep_num2 >> 16;
                            bin[51] = sleep_num2 >> 24;
                        }
                        bin[52] = ui->pr->value();
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

void Smartbuilding22::on_getconfig_clicked () {
    if (btnStatus) {
        ui->tips->setText("用户终止操作");
        CloseSerial();
        return;
    }
    int index = ui->devicetype->currentIndex();
    if (index != 0 && index != 1 && index != 2) {
        ui->tips->setText("该设备不支持读取");
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

void Smartbuilding22::on_setconfig_clicked () {
    if (btnStatus) {
        ui->tips->setText("用户终止操作");
        CloseSerial();
        return;
    }
    int index = ui->devicetype->currentIndex();
    if (index != 0 && index != 1 && index != 2) {
        retrytime = 0;
        chipstep = ISP_DEVICEMODE;
        if (OpenSerial()) {
            ui->tips->setText("串口打开失败");
            return;
        }
        btnStatus = 1;
        serial.write("AT+DEVICEMODE=0\r\n", 17);
    } else {
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
}

void Smartbuilding22::SetWriteSleepNum (int index, bool checked) {
    if ((index == 0 || index == 1) && checked) {
        ui->sleep_num1->setEnabled(true);
        ui->sleep_num2->setEnabled(true);
        writesleepnum = 1;
    } else {
        ui->sleep_num1->setEnabled(false);
        ui->sleep_num2->setEnabled(false);
        writesleepnum = 0;
    }
}

void Smartbuilding22::on_devicetype_currentIndexChanged(int index) {
    SetWriteSleepNum(index, ui->enablesleepnum->isChecked());
    if (index == 0 || index == 1) {
        ui->uploadmode->setEnabled(false);
        ui->pr->setEnabled(true);
        ui->enablesleepnum->setEnabled(true);
    } else {
        ui->uploadmode->setEnabled(true);
        ui->pr->setEnabled(false);
        ui->enablesleepnum->setEnabled(false);
    }
}

void Smartbuilding22::on_enablesleepnum_stateChanged(int param) {
    SetWriteSleepNum(ui->devicetype->currentIndex(), param);
}
