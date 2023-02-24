#include "tkmisp.h"
#include "ui_tkmisp.h"
#include <QFileDialog>
#include "common/hextobin.h"
#include "common/common.h"
// yyjson库
#include "common/yyjson.h"

#define ISP_SYNC                    0x7f
#define ISP_SYNC_TWO                0x8f
#define ISP_SYNC_THREE              0x8e
#define ISP_CHECK                   0x9f
#define ISP_ERASE                   0x0c
#define ISP_ERASE_ACK               0x0d
#define ISP_WRITE                   0x02
#define ISP_WRITE_ACK               0x03
#define ISP_READ                    0x08
#define ISP_READ_ACK                0x09
#define ISP_GETINFO                 0x00
#define ISP_GETINFO_ACK             0x01
#define ISP_CHANGE_BAUDRATE         0x12
#define ISP_CHANGE_BAUDRATE_ACK     0x13

#define BTN_STATUS_IDLE             0x00
#define BTN_STATUS_WRITE            0x01
#define BTN_STATUS_READ             0x02
#define BTN_STATUS_ERASE            0x03
#define BTN_STATUS_READ_MSG         0x04

TkmIsp::TkmIsp(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TkmIsp) {
    memcpy(failmsg, "fail", 4);
    memcpy(successmsg, "success", 7);
    ui->setupUi(this);
    GetComList();
}

TkmIsp::~TkmIsp() {
    delete ui;
}

// 获取可用的串口号
void TkmIsp::GetComList () {
    QComboBox *c = ui->com;
    c->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        c->addItem(info.portName() + " " + info.description());
    }
}

void TkmIsp::on_refresh_clicked () {
    if (btnStatus) {
        ui->tips->appendPlainText("用户终止操作");
        CloseSerial();
        return;
    }
    GetComList();
}

void TkmIsp::TimerOutEvent () {
    if (chipstep == ISP_SYNC || chipstep == ISP_SYNC_TWO) { // sync
        retrytime++;
        if (retrytime < 250) {
            char buff[64];
            QTextCursor pos = ui->tips->textCursor();
            pos.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
            pos.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            pos.removeSelectedText();
            pos.deletePreviousChar();
            sprintf(buff, "等待设备连接...%u", retrytime);
            ui->tips->appendPlainText(buff);
            return;
        }
        ui->tips->appendPlainText("串口同步失败");
        char dat[] = "syncfail";
        SendSocketData(dat, sizeof(dat)-1);
    } else {
        ui->tips->appendPlainText("设备响应超时");
        char dat[] = "timeout";
        SendSocketData(dat, sizeof(dat)-1);
    }
    CloseSerial();
}

void TkmIsp::SerialErrorEvent () {
    ui->tips->appendPlainText("串口错误");
    ui->tips->appendPlainText("");
    CloseSerial();
    char dat[] = "comerror";
    SendSocketData(dat, sizeof(dat)-1);
    GetComList();
}

int TkmIsp::GetBtnStatus () {
    return btnStatus + netctlStatus;
}

void TkmIsp::ReadSerialData () {
    QByteArray arr = serial.readAll();
    int len = arr.length();
    char *c = arr.data();
    memcpy(serialReadBuff+bufflen, c, len);
    bufflen += len;
    if (chipstep == ISP_SYNC) {
        if (bufflen != 8 || memcmp(serialReadBuff, "TurMass.", 8)) {
            bufflen = 0;
            return;
        }
        retrytime = 0;
        bufflen = 0;
        chipstep = ISP_SYNC_TWO;
        const char buff[] = "TaoLink.";
        serial.write(buff, 8);
    } else if (chipstep == ISP_SYNC_TWO) { // sync2
        if (bufflen != 2 || memcmp(serialReadBuff, "ok", 2)) {
            chipstep = ISP_SYNC;
            bufflen = 0;
            return;
        }
        bufflen = 0;
        retrytime = 0;
        ui->tips->appendPlainText("串口同步成功，开始修改串口波特率");
        char dat[] = "syncok";
        SendSocketData(dat, sizeof(dat)-1);
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
                ui->tips->appendPlainText("修改波特率失败");
                CloseSerial();
                SendSocketData(failmsg, sizeof(failmsg));
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
                ui->tips->appendPlainText(message);
                CloseSerial();
                SendSocketData(failmsg, sizeof(failmsg));
                return;
            }
            serial.clear();
            connect(&serial, SIGNAL(readyRead()), this, SLOT(ReadSerialData()));
            connect(&serial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(SerialErrorEvent()));
            if (btnStatus == BTN_STATUS_READ_MSG) {
                ui->tips->appendPlainText("串口波特率修改成功，开始获取芯片信息");
                chipstep = ISP_GETINFO;
                char buff[7];
                buff[0] = ISP_GETINFO;
                memset(buff+1, 0, 5);
                serial.write(buff, 7);
            } else if (btnStatus == BTN_STATUS_WRITE || btnStatus == BTN_STATUS_ERASE) {
                ui->tips->appendPlainText("串口波特率修改成功，开始擦除操作");
                chipstep = ISP_ERASE;
                char buff[7];
                buff[0] = ISP_ERASE;
                if (btnStatus == BTN_STATUS_ERASE) {
                    addr = eraseStart;
                } else if (bin1len > 0) {
                    addr = 0x00000000;
                } else if (bin0len > 0) {
                    addr = 0x00008000;
                } else {
                    char message[64];
                    sprintf(message, "程序内部错误%d", __LINE__);
                    ui->tips->appendPlainText(message);
                    CloseSerial();
                    SendSocketData(failmsg, sizeof(failmsg));
                    return;
                }
                offset = 0;
                buff[4] = addr>>24;
                buff[3] = addr>>16;
                buff[2] = addr>>8;
                buff[1] = addr;
                buff[6] = 0;
                buff[5] = 0;
                serial.write(buff, 7);
            } else if (btnStatus == BTN_STATUS_READ) {
                chipstep = ISP_READ;
                offset = 0;
                uint len = 0;
                if (bin1len > 0) {
                    ui->tips->appendPlainText("串口波特率修改成功，开始读取msu1操作");
                    addr = 0x00010000;
                    len = bin1len > 0x300 ? 0x300 : bin1len;
                } else if (bin0len > 0) {
                    ui->tips->appendPlainText("串口波特率修改成功，开始读取msu0操作");
                    addr = 0x00020000;
                    len = bin0len > 0x300 ? 0x300 : bin0len;
                } else {
                    char message[64];
                    sprintf(message, "程序内部错误%d", __LINE__);
                    ui->tips->appendPlainText(message);
                    CloseSerial();
                    return;
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
            } else if (btnStatus == BTN_STATUS_READ_MSG) {
                ui->tips->appendPlainText("串口波特率修改成功，开始读取芯片信息操作");
                chipstep = ISP_GETINFO;
            } else {
                char message[64];
                sprintf(message, "程序内部错误%d", __LINE__);
                ui->tips->appendPlainText(message);
                CloseSerial();
                return;
            }
        }
    } else if (chipstep == ISP_GETINFO) { // ISP_GETINFO
        if (serialReadBuff[0] != ISP_GETINFO_ACK) {
            retrytime++;
            if (retrytime == 250) {
                ui->tips->appendPlainText("读取失败");
                CloseSerial();
                return;
            }
            bufflen = 0;
            char buff[7];
            buff[0] = ISP_GETINFO;
            memset(buff+1, 0, 5);
            serial.write(buff, 7);
        } else if (bufflen == 7) {
            retrytime = 0;
            bufflen = 0;
            char buff[128];
            const char *memorytype;
            if (serialReadBuff[1] & 0x01) {
                memorytype = "EEPROM";
            } else if (serialReadBuff[1] & 0x02) {
                memorytype = "FLASH";
            } else {
                memorytype = "UNKNOWN";
            }
            sprintf(buff, "存储类型：%s，当年第%d颗，%d年，系列：%d", memorytype, (int)serialReadBuff[2] + 1, (int)serialReadBuff[3] + 2020, serialReadBuff[4]);
            ui->tips->appendPlainText(buff);
            CloseSerial();
            return;
        }
    } else if (chipstep == ISP_READ) { // ISP_READ
        if (serialReadBuff[0] != ISP_READ_ACK) {
            retrytime++;
            if (retrytime == 250) {
                ui->tips->appendPlainText("读取失败");
                CloseSerial();
                return;
            }
            bufflen = 0;
            offset = 0;
            uint len;
            if (bin1len > 0) {
                addr = 0x00010000;
                len = bin1len > 0x300 ? 0x300 : bin1len;
            } else if (bin0len > 0) {
                addr = 0x00020000;
                len = bin0len > 0x300 ? 0x300 : bin0len;
            } else {
                char message[64];
                sprintf(message, "程序内部错误%d", __LINE__);
                ui->tips->appendPlainText(message);
                CloseSerial();
                return;
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
        } else if (bufflen >= 7) {
            retrytime = 0;
            uchar address[4];
            address[3] = addr>>24;
            address[2] = addr>>16;
            address[1] = addr>>8;
            address[0] = addr;
            if (memcmp(address, serialReadBuff+1, 4)) {
                ui->tips->appendPlainText("读取失败");
                CloseSerial();
                return;
            }
            uint len = 256*serialReadBuff[6] + serialReadBuff[5];
            if (bufflen == len + 7) { // 获取数据完毕
                uchar *bin;
                uint binlen;
                uint partitionOneSize;
                uint partitionTwoStartPosition;
                if ((0x10000 <= addr && addr < 0x20000) || (0x40000 <= addr && addr < 0x48000)) {
                    bin = bin1;
                    binlen = bin1len;
                    partitionOneSize = 0x10000;
                    partitionTwoStartPosition = 0x40000;
                } else if ((0x20000 <= addr && addr < 0x40000) || (0x48000 <= addr && addr < 0x68000)) {
                    bin = bin0;
                    binlen = bin0len;
                    partitionOneSize = 0x20000;
                    partitionTwoStartPosition = 0x48000;
                } else {
                    char message[64];
                    sprintf(message, "程序内部错误%d", __LINE__);
                    ui->tips->appendPlainText(message);
                    CloseSerial();
                    return;
                }
                memcpy(bin+offset, serialReadBuff+7, len);
                offset += len;
                addr += len;
                if (offset == partitionOneSize) {
                    addr = partitionTwoStartPosition;
                }
                if (binlen < partitionOneSize || addr >= partitionTwoStartPosition) {
                    if (binlen - offset < 0x300) {
                        len = binlen - offset;
                    } else {
                        len = 0x300;
                    }
                } else {
                    if (partitionOneSize - offset < 0x300) {
                        len = partitionOneSize - offset;
                    } else {
                        len = 0x300;
                    }
                }
                if (len == 0) { // 这里将文件写入
                    char buff[64];
                    if (COMMON::filewrite(savefilepath, (char*)bin, offset, buff, 0)) {
                        ui->tips->appendPlainText(buff);
                        CloseSerial();
                    }
                    sprintf(buff, "全部读出完成，一共读出%uk数据", offset / 1024);
                    ui->tips->appendPlainText(buff);
                    CloseSerial();
                    return;
                }
                char buff[64];
                QTextCursor pos = ui->tips->textCursor();
                pos.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
                pos.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
                pos.removeSelectedText();
                pos.deletePreviousChar();
                sprintf(buff, "已读出%uk数据", offset / 1024);
                ui->tips->appendPlainText(buff);
                bufflen = 0;
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
    } else if (chipstep == ISP_ERASE) { // ISP_ERASE
        if (serialReadBuff[0] != ISP_ERASE_ACK) {
            retrytime++;
            if (retrytime == 250) {
                ui->tips->appendPlainText("擦除失败");
                CloseSerial();
                SendSocketData(failmsg, sizeof(failmsg));
                return;
            }
            if (btnStatus == BTN_STATUS_ERASE) {
                addr = eraseStart;
            } else if (bin1len > 0) {
                addr = 0x00000000;
            } else if (bin0len > 0) {
                addr = 0x00008000;
            } else {
                char message[64];
                sprintf(message, "程序内部错误%d", __LINE__);
                ui->tips->appendPlainText(message);
                SendSocketData(failmsg, sizeof(failmsg));
                return;
            }
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
        } else if (bufflen == 7) {
            retrytime = 0;
            bufflen = 0;
            uchar address[4];
            address[3] = addr>>24;
            address[2] = addr>>16;
            address[1] = addr>>8;
            address[0] = addr;
            if (memcmp(address, serialReadBuff+1, 4) || serialReadBuff[5] != 0x00 || serialReadBuff[6] != 0x00) {
                ui->tips->appendPlainText("擦除失败");
                CloseSerial();
                SendSocketData(failmsg, sizeof(failmsg));
                return;
            }
            char buff[128];
            uchar *bin;
            uint binlen;
            uint partitionParamEndPosition;
            uint partitionOneStartPosition;
            uint partitionOneEndPosition;
            uint partitionTwoStartPosition;
            if (btnStatus == BTN_STATUS_ERASE) {
                bin = NULL;
                binlen = eraseEnd - eraseStart;
                partitionParamEndPosition = eraseEnd;
                partitionOneStartPosition = eraseEnd;
                partitionOneEndPosition = eraseEnd;
                partitionTwoStartPosition = eraseEnd;
            } else if (addr < 0x8000 || (0x10000 <= addr && addr < 0x20000) || (0x40000 <= addr && addr < 0x48000)) { // msu1
                bin = bin1;
                binlen = bin1len + 0x8000;
                partitionParamEndPosition = 0x08000;
                partitionOneStartPosition = 0x10000;
                partitionOneEndPosition = 0x20000;
                partitionTwoStartPosition = 0x40000;
            } else if ((0x8000 <= addr && addr < 0x10000) || (0x20000 <= addr && addr < 0x40000) || (0x48000 <= addr && addr < 0x68000)) { // msu0
                bin = bin0;
                binlen = bin0len + 0x8000;
                partitionParamEndPosition = 0x10000;
                partitionOneStartPosition = 0x20000;
                partitionOneEndPosition = 0x40000;
                partitionTwoStartPosition = 0x48000;
            } else {
                char message[64];
                sprintf(message, "程序内部错误%d", __LINE__);
                ui->tips->appendPlainText(message);
                CloseSerial();
                SendSocketData(failmsg, sizeof(failmsg));
                return;
            }
            offset += 0x1000;
            QTextCursor pos = ui->tips->textCursor();
            pos.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
            pos.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            pos.removeSelectedText();
            pos.deletePreviousChar();
            if (offset < binlen) {
                if (bin == bin1) {
                    sprintf(buff, "已擦除msu1 %uk数据", offset / 1024);
                } else if (bin == bin0) {
                    sprintf(buff, "已擦除msu0 %uk数据", offset / 1024);
                } else {
                    sprintf(buff, "已擦除%uk数据", offset / 1024);
                }
                ui->tips->appendPlainText(buff);
                addr += 0x1000;
                if (addr == partitionParamEndPosition) {
                    addr = partitionOneStartPosition;
                } else if (addr == partitionOneEndPosition) {
                    addr = partitionTwoStartPosition;
                }
                buff[0] = ISP_ERASE;
                buff[4] = addr>>24;
                buff[3] = addr>>16;
                buff[2] = addr>>8;
                buff[1] = addr;
                buff[6] = 0;
                buff[5] = 0;
                serial.write(buff, 7);
            } else if (btnStatus == BTN_STATUS_ERASE) {
                sprintf(buff, "全部擦除完成，一共擦除%uk数据", offset / 1024);
                ui->tips->appendPlainText(buff);
                CloseSerial();
                SendSocketData(successmsg, sizeof(successmsg));
                return;
            } else if (bin == bin1 && bin0len > 0) { // 擦除msu1完毕且需要继续擦除msu0
                sprintf(buff, "擦除msu1完毕，一共擦除%uk数据", offset / 1024);
                ui->tips->appendPlainText(buff);
                ui->tips->appendPlainText("开始擦除msu0");
                addr = 0x00008000;
                offset = 0;
                buff[0] = ISP_ERASE;
                buff[4] = addr>>24;
                buff[3] = addr>>16;
                buff[2] = addr>>8;
                buff[1] = addr;
                buff[6] = 0;
                buff[5] = 0;
                serial.write(buff, 7);
            } else {
                if (bin == bin1) {
                    sprintf(buff, "擦除msu1完毕，一共擦除%uk数据", offset / 1024);
                } else if (bin == bin0) {
                    sprintf(buff, "擦除msu0完毕，一共擦除%uk数据", offset / 1024);
                } else {
                    sprintf(buff, "全部擦除完毕，一共擦除%uk数据", offset / 1024);
                }
                ui->tips->appendPlainText(buff);
                ui->tips->appendPlainText("开始写入数据");
                offset = 0;
                uint len;
                uchar param[0x1c];
                memset(param, 0xff, 0x1c);
                if (bin1len > 0) {
                    addr = 0x00000000;
                    param[0x00] = 0x06;
                    param[0x01] = 0x02;
                    param[0x02] = 0x20;
                    param[0x03] = 0x00;
                    param[0x04] = 0x00;
                    param[0x05] = 0x90;
                    param[0x06] = 0xd0;
                    param[0x07] = 0x03;
                    param[0x08] = bin1[0xfffc];
                    param[0x09] = bin1[0xfffd];
                    param[0x0a] = bin1[0xfffe];
                    param[0x0b] = bin1[0xffff];
                    param[0x10] = 0x01;
                    param[0x11] = 0x02;
                    param[0x12] = 0x40;
                    param[0x13] = 0x00;
                    param[0x14] = 0x51;
                    param[0x15] = 0x52;
                    param[0x16] = 0x52;
                    param[0x17] = 0x51;
                    param[0x18] = bin1[0x17ffc];
                    param[0x19] = bin1[0x17ffd];
                    param[0x1a] = bin1[0x17ffe];
                    param[0x1b] = bin1[0x17fff];
                } else if (bin0len > 0) {
                    addr = 0x00008000;
                    param[0x00] = 0x06;
                    param[0x01] = 0x02;
                    param[0x02] = 0x20;
                    param[0x03] = 0x00;
                    param[0x04] = 0x00;
                    param[0x05] = 0x90;
                    param[0x06] = 0xd0;
                    param[0x07] = 0x03;
                    param[0x08] = bin0[0x1fffc];
                    param[0x09] = bin0[0x1fffd];
                    param[0x0a] = bin0[0x1fffe];
                    param[0x0b] = bin0[0x1ffff];
                    param[0x10] = 0x01;
                    param[0x11] = 0x02;
                    param[0x12] = 0x40;
                    param[0x13] = 0x00;
                    param[0x14] = 0x51;
                    param[0x15] = 0x52;
                    param[0x16] = 0x52;
                    param[0x17] = 0x51;
                    param[0x18] = bin0[0x3fffc];
                    param[0x19] = bin0[0x3fffd];
                    param[0x1a] = bin0[0x3fffe];
                    param[0x1b] = bin0[0x3ffff];
                } else {
                    char message[64];
                    sprintf(message, "程序内部错误%d", __LINE__);
                    ui->tips->appendPlainText(message);
                    CloseSerial();
                    SendSocketData(failmsg, sizeof(failmsg));
                    return;
                }
                chipstep = ISP_WRITE;
                char *serialWriteBuff = (char*)malloc(7 + 0x1c);
                if (serialWriteBuff == NULL) {
                    ui->tips->appendPlainText("内存申请失败");
                    CloseSerial();
                    SendSocketData(failmsg, sizeof(failmsg));
                    return;
                }
                len = 0x1c;
                serialWriteBuff[0] = ISP_WRITE;
                serialWriteBuff[4] = addr >> 24;
                serialWriteBuff[3] = addr >> 16;
                serialWriteBuff[2] = addr >> 8;
                serialWriteBuff[1] = addr;
                serialWriteBuff[6] = len >> 8;
                serialWriteBuff[5] = len;
                memcpy(serialWriteBuff + 7, param, 0x1c);
                serial.write(serialWriteBuff, 0x1c+7);
                free(serialWriteBuff);
            }
        }
    } else if (chipstep == ISP_WRITE) { // ISP_WRITE
        if (serialReadBuff[0] != ISP_WRITE_ACK) {
            retrytime++;
            if (retrytime == 250) {
                ui->tips->appendPlainText("写入失败");
                CloseSerial();
                SendSocketData(failmsg, sizeof(failmsg));
                return;
            }
            bufflen = 0;
            offset = 0;
            chipstep = ISP_WRITE;
            uint len;
            uchar *bin;
            uchar param[0x1c];
            memset(param, 0xff, 0x1c);
            if (bin1len > 0) {
                addr = 0x00000000;
                param[0x00] = 0x06;
                param[0x01] = 0x02;
                param[0x02] = 0x20;
                param[0x03] = 0x00;
                param[0x04] = 0x00;
                param[0x05] = 0x90;
                param[0x06] = 0xd0;
                param[0x07] = 0x03;
                param[0x08] = bin1[0xfffc];
                param[0x09] = bin1[0xfffd];
                param[0x0a] = bin1[0xfffe];
                param[0x0b] = bin1[0xffff];
                param[0x10] = 0x01;
                param[0x11] = 0x02;
                param[0x12] = 0x40;
                param[0x13] = 0x00;
                param[0x14] = 0x51;
                param[0x15] = 0x52;
                param[0x16] = 0x52;
                param[0x17] = 0x51;
                param[0x18] = bin1[0x17ffc];
                param[0x19] = bin1[0x17ffd];
                param[0x1a] = bin1[0x17ffe];
                param[0x1b] = bin1[0x17fff];
                bin = param;
                len = 0x1c;
            } else if (bin0len > 0) {
                addr = 0x00008000;
                param[0x00] = 0x06;
                param[0x01] = 0x02;
                param[0x02] = 0x20;
                param[0x03] = 0x00;
                param[0x04] = 0x00;
                param[0x05] = 0x90;
                param[0x06] = 0xd0;
                param[0x07] = 0x03;
                param[0x08] = bin0[0x1fffc];
                param[0x09] = bin0[0x1fffd];
                param[0x0a] = bin0[0x1fffe];
                param[0x0b] = bin0[0x1ffff];
                param[0x10] = 0x01;
                param[0x11] = 0x02;
                param[0x12] = 0x40;
                param[0x13] = 0x00;
                param[0x14] = 0x51;
                param[0x15] = 0x52;
                param[0x16] = 0x52;
                param[0x17] = 0x51;
                param[0x18] = bin0[0x3fffc];
                param[0x19] = bin0[0x3fffd];
                param[0x1a] = bin0[0x3fffe];
                param[0x1b] = bin0[0x3ffff];
                bin = param;
                len = 0x1c;
            } else {
                char message[64];
                sprintf(message, "程序内部错误%d", __LINE__);
                ui->tips->appendPlainText(message);
                CloseSerial();
                SendSocketData(failmsg, sizeof(failmsg));
                return;
            }
            char *serialWriteBuff = (char*)malloc(len+7);
            if (serialWriteBuff == NULL) {
                ui->tips->appendPlainText("内存申请失败");
                CloseSerial();
                SendSocketData(failmsg, sizeof(failmsg));
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
        } else if (bufflen == 7) {
            retrytime = 0;
            bufflen = 0;
            uchar address[4];
            address[3] = addr>>24;
            address[2] = addr>>16;
            address[1] = addr>>8;
            address[0] = addr;
            if (memcmp(address, serialReadBuff+1, 4)) {
                ui->tips->appendPlainText("写入数据失败");
                CloseSerial();
                SendSocketData(failmsg, sizeof(failmsg));
                return;
            }
            uint len = 256*serialReadBuff[6] + serialReadBuff[5];
            uchar *bin;
            uint binlen;
            uint partitionOneSize;
            uint partitionParamStartPosition;
            uint partitionOneStartPosition;
            uint partitionOneEndPosition;
            uint partitionTwoStartPosition;
            uchar param[0x1c];
            if (addr == 0x0000 || (0x10000 <= addr && addr < 0x20000) || (0x40000 <= addr && addr < 0x48000)) { // msu0
                bin = bin1;
                binlen = bin1len;
                partitionOneSize = 0x10000;
                partitionParamStartPosition = 0x0000;
                partitionOneStartPosition = 0x10000;
                partitionOneEndPosition = 0x20000;
                partitionTwoStartPosition = 0x40000;
            } else if (addr == 0x8000 || (0x20000 <= addr && addr < 0x40000) || (0x48000 <= addr && addr < 0x68000)) { // msu0
                bin = bin0;
                binlen = bin0len;
                partitionOneSize = 0x20000;
                partitionParamStartPosition = 0x8000;
                partitionOneStartPosition = 0x20000;
                partitionOneEndPosition = 0x40000;
                partitionTwoStartPosition = 0x48000;
            } else {
                char message[64];
                sprintf(message, "程序内部错误%d", __LINE__);
                ui->tips->appendPlainText(message);
                CloseSerial();
                SendSocketData(failmsg, sizeof(failmsg));
                return;
            }
            if (addr != 0x0000 && addr != 0x8000) {
                offset += len;
                addr += len;
            }
            if (addr == partitionParamStartPosition) {
                addr = partitionOneStartPosition;
            } else if (addr == partitionOneEndPosition) {
                addr = partitionTwoStartPosition;
            }
            if (binlen < partitionOneSize || addr >= partitionTwoStartPosition) {
                if (binlen - offset < 0x100) {
                    len = binlen - offset;
                } else {
                    len = 0x100;
                }
            } else {
                if (partitionOneSize - offset < 0x100) {
                    len = partitionOneSize - offset;
                } else {
                    len = 0x100;
                }
            }
            bool startcheck = false;
            char buff[64];
            QTextCursor pos = ui->tips->textCursor();
            pos.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
            pos.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            pos.removeSelectedText();
            pos.deletePreviousChar();
            if (len == 0) { // 烧录完毕
                if (bin == bin1 && bin0len > 0) { // 写入msu1完毕且需要继续写入msu0
                    sprintf(buff, "写入msu1完毕，一共写入%uk数据", offset / 1024);
                    ui->tips->appendPlainText(buff);
                    ui->tips->appendPlainText("开始写入msu0");
                    bin = bin0;
                    addr = 0x00008000;
                    memset(param, 0xff, 0x1c);
                    param[0x00] = 0x06;
                    param[0x01] = 0x02;
                    param[0x02] = 0x20;
                    param[0x03] = 0x00;
                    param[0x04] = 0x00;
                    param[0x05] = 0x90;
                    param[0x06] = 0xd0;
                    param[0x07] = 0x03;
                    param[0x08] = bin0[0x1fffc];
                    param[0x09] = bin0[0x1fffd];
                    param[0x0a] = bin0[0x1fffe];
                    param[0x0b] = bin0[0x1ffff];
                    param[0x10] = 0x01;
                    param[0x11] = 0x02;
                    param[0x12] = 0x40;
                    param[0x13] = 0x00;
                    param[0x14] = 0x51;
                    param[0x15] = 0x52;
                    param[0x16] = 0x52;
                    param[0x17] = 0x51;
                    param[0x18] = bin0[0x3fffc];
                    param[0x19] = bin0[0x3fffd];
                    param[0x1a] = bin0[0x3fffe];
                    param[0x1b] = bin0[0x3ffff];
                    bin = param;
                    len = 0x1c;
                    offset = 0;
                } else {
                    if (bin == bin1) {
                        sprintf(buff, "写入msu1完毕，一共写入%uk数据", offset / 1024);
                    } else if (bin == bin0) {
                        sprintf(buff, "写入msu0完毕，一共写入%uk数据", offset / 1024);
                    } else {
                        sprintf(buff, "写入完毕，共写入%uk数据", offset / 1024);
                    }
                    ui->tips->appendPlainText(buff);
                    if (needcheck) {
                        startcheck = true;
                    } else {
                        CloseSerial();
                        SendSocketData(successmsg, sizeof(successmsg));
                        return;
                    }
                }
            } else {
                if (bin == bin1) {
                    sprintf(buff, "已写入msu1 %uk数据", offset / 1024);
                } else if (bin == bin0) {
                    sprintf(buff, "已写入msu0 %uk数据", offset / 1024);
                } else {
                    sprintf(buff, "已写入%uk数据", offset / 1024);
                }
                ui->tips->appendPlainText(buff);
            }
            if (startcheck) {
                ui->tips->appendPlainText("开始校验数据");
                chipstep = ISP_CHECK;
                offset = 0;
                uint len = 0x1c;
                if (bin1len > 0) {
                    addr = 0x00000000;
                } else if (bin0len > 0) {
                    addr = 0x00008000;
                } else {
                    char message[64];
                    sprintf(message, "程序内部错误%d", __LINE__);
                    ui->tips->appendPlainText(message);
                    CloseSerial();
                    SendSocketData(failmsg, sizeof(failmsg));
                    return;
                }
                buff[0] = ISP_READ;
                buff[4] = addr>>24;
                buff[3] = addr>>16;
                buff[2] = addr>>8;
                buff[1] = addr;
                buff[6] = len >> 8;
                buff[5] = len;
                serial.write(buff, 7);
            } else {
                char *serialWriteBuff = (char*)malloc(len+7);
                if (serialWriteBuff == NULL) {
                    ui->tips->appendPlainText("内存申请失败");
                    CloseSerial();
                    SendSocketData(failmsg, sizeof(failmsg));
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
        }
    } else if (chipstep == ISP_CHECK) { // ISP_CHECK
        if (serialReadBuff[0] != ISP_READ_ACK) {
            retrytime++;
            if (retrytime == 250) {
                ui->tips->appendPlainText("校验失败");
                CloseSerial();
                SendSocketData(failmsg, sizeof(failmsg));
                return;
            }
            offset = 0;
            bufflen = 0;
            uint len = 0x1c;
            if (bin1len > 0) {
                addr = 0x00000000;
            } else if (bin0len > 0) {
                addr = 0x00008000;
            } else {
                char message[64];
                sprintf(message, "程序内部错误%d", __LINE__);
                ui->tips->appendPlainText(message);
                CloseSerial();
                SendSocketData(failmsg, sizeof(failmsg));
                return;
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
        } else if (bufflen >= 7) {
            retrytime = 0;
            uchar address[4];
            address[3] = addr>>24;
            address[2] = addr>>16;
            address[1] = addr>>8;
            address[0] = addr;
            if (memcmp(address, serialReadBuff+1, 4)) {
                ui->tips->appendPlainText("校验失败");
                CloseSerial();
                SendSocketData(failmsg, sizeof(failmsg));
                return;
            }
            uint len = 256*serialReadBuff[6] + serialReadBuff[5];
            if (bufflen == len + 7) { // 获取数据完毕
                uchar *bin;
                uint binlen;
                uint partitionOneSize;
                uint partitionParamStartPosition;
                uint partitionOneStartPosition;
                uint partitionOneEndPosition;
                uint partitionTwoStartPosition;
                if (addr == 0x0000 || (0x10000 <= addr && addr < 0x20000) || (0x40000 <= addr && addr < 0x48000)) {
                    bin = bin1;
                    binlen = bin1len;
                    partitionOneSize = 0x10000;
                    partitionParamStartPosition = 0x0000;
                    partitionOneStartPosition = 0x10000;
                    partitionOneEndPosition = 0x20000;
                    partitionTwoStartPosition = 0x40000;
                } else if (addr == 0x8000 || (0x20000 <= addr && addr < 0x40000) || (0x48000 <= addr && addr < 0x68000)) {
                    bin = bin0;
                    binlen = bin0len;
                    partitionOneSize = 0x20000;
                    partitionParamStartPosition = 0x8000;
                    partitionOneStartPosition = 0x20000;
                    partitionOneEndPosition = 0x40000;
                    partitionTwoStartPosition = 0x48000;
                } else {
                    char message[64];
                    sprintf(message, "程序内部错误%d", __LINE__);
                    ui->tips->appendPlainText(message);
                    CloseSerial();
                    SendSocketData(failmsg, sizeof(failmsg));
                    return;
                }
                if (addr == partitionParamStartPosition) {
                    uchar param[0x1c];
                    memset(param, 0xff, 0x1c);
                    if (addr == 0x0000) {
                        param[0x00] = 0x06;
                        param[0x01] = 0x02;
                        param[0x02] = 0x20;
                        param[0x03] = 0x00;
                        param[0x04] = 0x00;
                        param[0x05] = 0x90;
                        param[0x06] = 0xd0;
                        param[0x07] = 0x03;
                        param[0x08] = bin[0xfffc];
                        param[0x09] = bin[0xfffd];
                        param[0x0a] = bin[0xfffe];
                        param[0x0b] = bin[0xffff];
                        param[0x10] = 0x01;
                        param[0x11] = 0x02;
                        param[0x12] = 0x40;
                        param[0x13] = 0x00;
                        param[0x14] = 0x51;
                        param[0x15] = 0x52;
                        param[0x16] = 0x52;
                        param[0x17] = 0x51;
                        param[0x18] = bin[0x17ffc];
                        param[0x19] = bin[0x17ffd];
                        param[0x1a] = bin[0x17ffe];
                        param[0x1b] = bin[0x17fff];
                    } else if (addr == 0x8000) {
                        param[0x00] = 0x06;
                        param[0x01] = 0x02;
                        param[0x02] = 0x20;
                        param[0x03] = 0x00;
                        param[0x04] = 0x00;
                        param[0x05] = 0x90;
                        param[0x06] = 0xd0;
                        param[0x07] = 0x03;
                        param[0x08] = bin[0x1fffc];
                        param[0x09] = bin[0x1fffd];
                        param[0x0a] = bin[0x1fffe];
                        param[0x0b] = bin[0x1ffff];
                        param[0x10] = 0x01;
                        param[0x11] = 0x02;
                        param[0x12] = 0x40;
                        param[0x13] = 0x00;
                        param[0x14] = 0x51;
                        param[0x15] = 0x52;
                        param[0x16] = 0x52;
                        param[0x17] = 0x51;
                        param[0x18] = bin[0x3fffc];
                        param[0x19] = bin[0x3fffd];
                        param[0x1a] = bin[0x3fffe];
                        param[0x1b] = bin[0x3ffff];
                    }
                    if (memcmp(param, serialReadBuff+7, 0x1c)) {
                        ui->tips->appendPlainText("程序参数校验错误");
                        CloseSerial();
                        SendSocketData(failmsg, sizeof(failmsg));
                        return;
                    }
                } else {
                    if (memcmp(bin+offset, serialReadBuff+7, len)) {
                        ui->tips->appendPlainText("程序校验错误");
                        CloseSerial();
                        SendSocketData(failmsg, sizeof(failmsg));
                        return;
                    }
                    offset += len;
                    addr += len;
                }
                if (addr == partitionParamStartPosition) {
                    addr = partitionOneStartPosition;
                } else if (addr == partitionOneEndPosition) {
                    addr = partitionTwoStartPosition;
                }
                if (binlen < partitionOneSize || addr >= partitionTwoStartPosition) {
                    if (binlen - offset < 0x300) {
                        len = binlen - offset;
                    } else {
                        len = 0x300;
                    }
                } else {
                    if (partitionOneSize - offset < 0x300) {
                        len = partitionOneSize - offset;
                    } else {
                        len = 0x300;
                    }
                }
                char buff[64];
                QTextCursor pos = ui->tips->textCursor();
                pos.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
                pos.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
                pos.removeSelectedText();
                pos.deletePreviousChar();
                if (len == 0) {
                    if (bin == bin1 && bin0len > 0) {
                        sprintf(buff, "校验msu1完毕，一共校验%uk数据", offset / 1024);
                        ui->tips->appendPlainText(buff);
                        ui->tips->appendPlainText("开始校验msu0");
                        addr = 0x00008000;
                        len = 0x1c;
                        offset = 0;
                    } else {
                        if (bin == bin1) {
                            sprintf(buff, "校验msu1完毕，一共校验%uk数据", offset / 1024);
                        } else if (bin == bin0) {
                            sprintf(buff, "校验msu0完毕，一共校验%uk数据", offset / 1024);
                        } else {
                            sprintf(buff, "校验完毕，共校验%uk数据", offset / 1024);
                        }
                        ui->tips->appendPlainText(buff);
                        CloseSerial();
                        SendSocketData(successmsg, sizeof(successmsg));
                        return;
                    }
                } else {
                    if (bin == bin1) {
                        sprintf(buff, "已校验msu1 %uk数据", offset / 1024);
                    } else if (bin == bin0) {
                        sprintf(buff, "已校验msu0 %uk数据", offset / 1024);
                    } else {
                        sprintf(buff, "已校验%uk数据", offset / 1024);
                    }
                    ui->tips->appendPlainText(buff);
                }
                bufflen = 0;
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
    } else {
        char buff[64];
        sprintf(buff, "chipstep:0x%02x, bufflen:%d, ack:0x%02x", chipstep, bufflen, serialReadBuff[0]);
        ui->tips->appendPlainText(buff);
        CloseSerial();
        SendSocketData(failmsg, sizeof(failmsg));
        return;
    }
    timer.stop();
    if (chipstep == ISP_SYNC) {
        timer.start(1000); // 正在同步，所需时间比较短
    } else {
        timer.start(3000); // 非ISP_SYNC命令，非擦除命令，统一等待3s
    }
}

int TkmIsp::OpenSerial () {
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

void TkmIsp::CloseSerial () {
    disconnect(&timer, 0, 0, 0);
    timer.stop();
    disconnect(&serial, 0, 0, 0);
    serial.close();
    bufflen = 0;
    btnStatus = BTN_STATUS_IDLE;
}

void TkmIsp::on_msu1openfile_clicked () {
    if (btnStatus) {
        ui->tips->appendPlainText("用户终止操作");
        CloseSerial();
        return;
    }
    QString filepath = QFileDialog::getOpenFileName(this, "选择镜像", NULL, "镜像文件(*.hex *.bin)");
    if (filepath.length()) {
        ui->msu1filepath->setText(filepath);
        ui->msu1load->setChecked(true);
    }
}

void TkmIsp::on_msu0openfile_clicked () {
    if (btnStatus) {
        ui->tips->appendPlainText("用户终止操作");
        CloseSerial();
        return;
    }
    QString filepath = QFileDialog::getOpenFileName(this, "选择镜像", NULL, "镜像文件(*.hex *.bin)");
    if (filepath.length()) {
        ui->msu0filepath->setText(filepath);
        ui->msu0load->setChecked(true);
    }
}

void TkmIsp::on_writechip_clicked () {
    if (btnStatus) {
        ui->tips->appendPlainText("用户终止操作");
        CloseSerial();
        return;
    }
    needcheck = ui->checkwrite->isChecked();
    char buff[64];
    bin0len = 0;
    bin1len = 0;
    if (ui->msu0load->isChecked()) {
        QString filepath = ui->msu0filepath->text();
        QFile file(filepath);
        if(!file.open(QIODevice::ReadOnly)) {
            ui->tips->appendPlainText("无法读取烧录文件");
            SendSocketData(failmsg, sizeof(failmsg));
            return;
        }
        QByteArray bytedata = file.readAll();
        file.close();
        int pos = filepath.lastIndexOf(".");
        QString suffix = filepath.mid(pos);
        uchar *chardata = (uchar*)bytedata.data();
        if (!suffix.compare(".hex")) {
            if (chardata[0] == ':') { // 标准ihex文件
                if (HexToBin(chardata, bytedata.length(), bin0, sizeof(bin0), 0x00000000, &bin0len) != RES_OK) {
                    ui->tips->appendPlainText("hex文件格式错误");
                    SendSocketData(failmsg, sizeof(failmsg));
                    return;
                }
            } else { // 本工程独有的hex文件
                int res = COMMON::TKM_HexToBin(chardata, bytedata.length(), bin0, sizeof(bin0), &bin0len);
                if (res == -1) {
                    ui->tips->appendPlainText("hex文件格式错误");
                    SendSocketData(failmsg, sizeof(failmsg));
                    return;
                } else if (res == -2 || bin0len != sizeof(bin0)) {
                    sprintf(buff, "镜像文件大小不等于%ukbytes", (uint)sizeof(bin0)/1024);
                    ui->tips->appendPlainText(buff);
                    SendSocketData(failmsg, sizeof(failmsg));
                    return;
                }
            }
        } else if (!suffix.compare(".bin")) {
            uint len = bytedata.length();
            if (len != sizeof(bin0)) {
                sprintf(buff, "镜像文件大小不等于%ukbytes", (uint)sizeof(bin0)/1024);
                ui->tips->appendPlainText(buff);
                SendSocketData(failmsg, sizeof(failmsg));
                return;
            }
            memcpy(bin0, chardata, len);
            bin0len = len;
        } else {
            ui->tips->appendPlainText("msu0文件类型错误");
            SendSocketData(failmsg, sizeof(failmsg));
            return;
        }
    }
    if (ui->msu1load->isChecked()) {
        QString filepath = ui->msu1filepath->text();
        QFile file(filepath);
        if(!file.open(QIODevice::ReadOnly)) {
            ui->tips->appendPlainText("无法读取烧录文件");
            SendSocketData(failmsg, sizeof(failmsg));
            return;
        }
        QByteArray bytedata = file.readAll();
        file.close();
        int pos = filepath.lastIndexOf(".");
        QString suffix = filepath.mid(pos);
        uchar *chardata = (uchar*)bytedata.data();
        if (!suffix.compare(".hex")) {
            if (chardata[0] == ':') { // 标准ihex文件
                if (HexToBin(chardata, bytedata.length(), bin1, sizeof(bin1), 0x00000000, &bin1len) != RES_OK) {
                    ui->tips->appendPlainText("hex文件格式错误");
                    SendSocketData(failmsg, sizeof(failmsg));
                    return;
                }
            } else { // 本工程独有的hex文件
                int res = COMMON::TKM_HexToBin(chardata, bytedata.length(), bin1, sizeof(bin1), &bin1len);
                if (res == -1) {
                    ui->tips->appendPlainText("hex文件格式错误");
                    SendSocketData(failmsg, sizeof(failmsg));
                    return;
                } else if (res == -2 || bin1len != sizeof(bin1)) {
                    sprintf(buff, "镜像文件大小不等于%ukbytes", (uint)sizeof(bin1)/1024);
                    ui->tips->appendPlainText(buff);
                    SendSocketData(failmsg, sizeof(failmsg));
                    return;
                }
            }
        } else if (!suffix.compare(".bin")) {
            uint len = bytedata.length();
            if (len != sizeof(bin1)) {
                sprintf(buff, "镜像文件大小不等于%ukbytes", (uint)sizeof(bin1)/1024);
                ui->tips->appendPlainText(buff);
                SendSocketData(failmsg, sizeof(failmsg));
                return;
            }
            memcpy(bin1, chardata, len);
            bin1len = len;
        } else {
            ui->tips->appendPlainText("msu0文件类型错误");
            SendSocketData(failmsg, sizeof(failmsg));
            return;
        }
    }
    if (bin0len == 0 && bin1len == 0) {
        ui->tips->appendPlainText("没有需要烧录的分区");
        SendSocketData(failmsg, sizeof(failmsg));
        return;
    }
    retrytime = 0;
    chipstep = ISP_SYNC; // sync
    buff[0] = ISP_SYNC;
    if (OpenSerial()) {
        ui->tips->appendPlainText("串口打开失败");
        SendSocketData(failmsg, sizeof(failmsg));
        return;
    }
    btnStatus = BTN_STATUS_WRITE;
    sprintf(buff, "等待设备连接...%u", ++retrytime);
    ui->tips->appendPlainText(buff);
}

void TkmIsp::on_erasechip_clicked () {
    if (btnStatus) {
        ui->tips->appendPlainText("用户终止操作");
        CloseSerial();
        return;
    }
    if (sscanf(ui->erasestart->text().toUtf8().data(), "0x%x", &eraseStart) != 1) {
        ui->tips->appendPlainText("擦除起始位置读取失败");
        return;
    }
    if (sscanf(ui->eraseend->text().toUtf8().data(), "0x%x", &eraseEnd) != 1) {
        ui->tips->appendPlainText("擦除终止位置读取失败");
        return;
    }
    if (eraseEnd <= eraseStart) {
        ui->tips->appendPlainText("终止位置需大于起始位置");
        return;
    }
    retrytime = 0;
    chipstep = ISP_SYNC; // sync
    char buff[64];
    buff[0] = ISP_SYNC;
    if (OpenSerial()) {
        ui->tips->appendPlainText("串口打开失败");
        SendSocketData(failmsg, sizeof(failmsg));
        return;
    }
    btnStatus = BTN_STATUS_ERASE;
    sprintf(buff, "等待设备连接...%u", ++retrytime);
    ui->tips->appendPlainText(buff);
}

void TkmIsp::on_clearlog_clicked () {
    ui->tips->clear();
}

void TkmIsp::on_msu0readchip_clicked () {
    if (btnStatus) {
        ui->tips->appendPlainText("用户终止操作");
        CloseSerial();
        return;
    }
    char buff[64];
    bin0len = sizeof(bin0);
    bin1len = 0;
    QString filepath = QFileDialog::getSaveFileName(this, "保存镜像", NULL, "镜像文件(*.bin)");
    if (!filepath.length()) {
        return;
    }
    savefilepath = filepath;
    retrytime = 0;
    chipstep = ISP_SYNC; // sync
    buff[0] = ISP_SYNC;
    if (OpenSerial()) {
        ui->tips->appendPlainText("串口打开失败");
        return;
    }
    btnStatus = BTN_STATUS_READ;
    sprintf(buff, "等待设备连接...%u", ++retrytime);
    ui->tips->appendPlainText(buff);
}

void TkmIsp::on_msu1readchip_clicked() {
    if (btnStatus) {
        ui->tips->appendPlainText("用户终止操作");
        CloseSerial();
        return;
    }
    char buff[64];
    bin1len = sizeof(bin1);
    bin0len = 0;
    QString filepath = QFileDialog::getSaveFileName(this, "保存镜像", NULL, "镜像文件(*.bin)");
    if (!filepath.length()) {
        return;
    }
    savefilepath = filepath;
    retrytime = 0;
    chipstep = ISP_SYNC; // sync
    buff[0] = ISP_SYNC;
    if (OpenSerial()) {
        ui->tips->appendPlainText("串口打开失败");
        return;
    }
    btnStatus = BTN_STATUS_READ;
    sprintf(buff, "等待设备连接...%u", ++retrytime);
    ui->tips->appendPlainText(buff);
}

void TkmIsp::on_readchipmsg_clicked() {
    if (btnStatus) {
        ui->tips->appendPlainText("用户终止操作");
        CloseSerial();
        return;
    }
    retrytime = 0;
    chipstep = ISP_SYNC; // sync
    char buff[64];
    buff[0] = ISP_SYNC;
    if (OpenSerial()) {
        ui->tips->appendPlainText("串口打开失败");
        return;
    }
    btnStatus = BTN_STATUS_READ_MSG;
    sprintf(buff, "等待设备连接...%u", ++retrytime);
    ui->tips->appendPlainText(buff);
}

void TkmIsp::SendSocketData (char *dat, int len) {
    if (netctlStatus) {
        udpSocket.writeDatagram(dat, len, srcAddress, srcPort);
    }
}

void TkmIsp::ReadSocketData () {
    char ba[1024];
    int size = 0;
    while(udpSocket.hasPendingDatagrams()) {
        int lastsize = size;
        int datasize = udpSocket.pendingDatagramSize();
        size += datasize;
        udpSocket.readDatagram(ba + lastsize, datasize, &srcAddress, &srcPort);
    }
    yyjson_doc *doc = yyjson_read(ba, size, 0);
    if (doc == NULL) {
        char dat[] = "json parse fail";
        udpSocket.writeDatagram(dat, sizeof(dat)-1, srcAddress, srcPort);
        return;
    }
    yyjson_val *json = yyjson_doc_get_root(doc);
    yyjson_val *act = yyjson_obj_get(json, "act");
    if (act == NULL || !yyjson_is_str(act)) {
        yyjson_doc_free(doc);
        char dat[] = "act not found";
        udpSocket.writeDatagram(dat, sizeof(dat)-1, srcAddress, srcPort);
        return;
    }
    if (!strcmp(yyjson_get_str(act), "erasechip")) {
        yyjson_doc_free(doc);
        if (btnStatus) {
            char dat[] = "busy...";
            udpSocket.writeDatagram(dat, sizeof(dat)-1, srcAddress, srcPort);
            return;
        }
        char dat[] = "wait";
        udpSocket.writeDatagram(dat, sizeof(dat)-1, srcAddress, srcPort);
        on_erasechip_clicked();
    } else if (!strcmp(yyjson_get_str(act), "writechip")) {
        yyjson_val *msu1path = yyjson_obj_get(json, "msu1path");
        if (msu1path != NULL && yyjson_is_str(msu1path)) {
            ui->msu1filepath->setText(yyjson_get_str(msu1path));
            ui->msu1load->setChecked(true);
        }
        yyjson_val *msu0path = yyjson_obj_get(json, "msu0path");
        if (msu0path != NULL && yyjson_is_str(msu0path)) {
            ui->msu0filepath->setText(yyjson_get_str(msu0path));
            ui->msu0load->setChecked(true);
        }
        yyjson_val *msu0load = yyjson_obj_get(json, "msu0load");
        if (msu0load != NULL && yyjson_is_bool(msu0load)) {
            ui->msu0load->setChecked(yyjson_get_bool(msu0load));
        }
        yyjson_val *msu1load = yyjson_obj_get(json, "msu1load");
        if (msu1load != NULL && yyjson_is_bool(msu1load)) {
            ui->msu1load->setChecked(yyjson_get_bool(msu1load));
        }
        yyjson_doc_free(doc);
        if (btnStatus) {
            char dat[] = "busy...";
            udpSocket.writeDatagram(dat, sizeof(dat)-1, srcAddress, srcPort);
            return;
        }
        char dat[] = "wait";
        udpSocket.writeDatagram(dat, sizeof(dat)-1, srcAddress, srcPort);
        on_writechip_clicked();
    } else if (!strcmp(yyjson_get_str(act), "setmsu0path")) {
        yyjson_val *msu0path = yyjson_obj_get(json, "msu0path");
        if (msu0path == NULL || !yyjson_is_str(msu0path)) {
            yyjson_doc_free(doc);
            char dat[] = "msu0path not found";
            udpSocket.writeDatagram(dat, sizeof(dat)-1, srcAddress, srcPort);
            return;
        }
        ui->msu0filepath->setText(yyjson_get_str(msu0path));
        ui->msu0load->setChecked(true);
        yyjson_doc_free(doc);
        udpSocket.writeDatagram(successmsg, sizeof(successmsg), srcAddress, srcPort);
    } else if (!strcmp(yyjson_get_str(act), "setmsu1path")) {
        yyjson_val *msu1path = yyjson_obj_get(json, "msu1path");
        if (msu1path == NULL || !yyjson_is_str(msu1path)) {
            yyjson_doc_free(doc);
            char dat[] = "msu1path not found";
            udpSocket.writeDatagram(dat, sizeof(dat)-1, srcAddress, srcPort);
            return;
        }
        ui->msu1filepath->setText(yyjson_get_str(msu1path));
        ui->msu1load->setChecked(true);
        yyjson_doc_free(doc);
        udpSocket.writeDatagram(successmsg, sizeof(successmsg), srcAddress, srcPort);
    } else if (!strcmp(yyjson_get_str(act), "setmsu0load")) {
        yyjson_val *msu0load = yyjson_obj_get(json, "msu0load");
        if (msu0load == NULL || !yyjson_is_bool(msu0load)) {
            yyjson_doc_free(doc);
            char dat[] = "msu0load not found";
            udpSocket.writeDatagram(dat, sizeof(dat)-1, srcAddress, srcPort);
            return;
        }
        ui->msu0load->setChecked(yyjson_get_bool(msu0load));
        yyjson_doc_free(doc);
        udpSocket.writeDatagram(successmsg, sizeof(successmsg), srcAddress, srcPort);
    } else if (!strcmp(yyjson_get_str(act), "setmsu1load")) {
        yyjson_val *msu0load = yyjson_obj_get(json, "msu1path");
        if (msu0load == NULL || !yyjson_is_bool(msu0load)) {
            yyjson_doc_free(doc);
            char dat[] = "msu0load not found";
            udpSocket.writeDatagram(dat, sizeof(dat)-1, srcAddress, srcPort);
            return;
        }
        ui->msu0load->setChecked(yyjson_get_bool(msu0load));
        yyjson_doc_free(doc);
        udpSocket.writeDatagram(successmsg, sizeof(successmsg), srcAddress, srcPort);
    } else if (!strcmp(yyjson_get_str(act), "stop")) {
        yyjson_doc_free(doc);
        if (btnStatus) {
            ui->tips->appendPlainText("用户终止操作");
            CloseSerial();
        }
        udpSocket.writeDatagram(successmsg, sizeof(successmsg), srcAddress, srcPort);
    } else if (!strcmp(yyjson_get_str(act), "clearlog")) {
        yyjson_doc_free(doc);
        on_clearlog_clicked();
        udpSocket.writeDatagram(successmsg, sizeof(successmsg), srcAddress, srcPort);
    } else {
        yyjson_doc_free(doc);
        char dat[] = "unknown";
        udpSocket.writeDatagram(dat, sizeof(dat)-1, srcAddress, srcPort);
    }
}

void TkmIsp::on_netctl_clicked() {
    if (netctlStatus == 0) {
        if (!udpSocket.open(QIODevice::ReadWrite)) {
            ui->tips->appendPlainText("打开套接字失败");
            return;
        }
        if (!udpSocket.bind(QHostAddress::Any, 33234)) {
            udpSocket.close();
            ui->tips->appendPlainText("绑定33234端口失败");
            return;
        }
        ui->writechip->setEnabled(false);
        ui->erasechip->setEnabled(false);
        ui->checkwrite->setEnabled(false);
        ui->msu0readchip->setEnabled(false);
        ui->msu1readchip->setEnabled(false);
        ui->readchipmsg->setEnabled(false);
        ui->msu0filepath->setEnabled(false);
        ui->msu1filepath->setEnabled(false);
        ui->msu0openfile->setEnabled(false);
        ui->msu1openfile->setEnabled(false);
        ui->msu0load->setEnabled(false);
        ui->msu1load->setEnabled(false);
        ui->netctl->setText("关闭远程调用");
        connect(&udpSocket, SIGNAL(readyRead()), this, SLOT(ReadSocketData()));
        netctlStatus = 1;
        ui->tips->appendPlainText("开启远程调用成功，监听端口为33234。");
    } else {
        udpSocket.close();
        ui->writechip->setEnabled(true);
        ui->erasechip->setEnabled(true);
        ui->checkwrite->setEnabled(true);
        ui->msu0readchip->setEnabled(true);
        ui->msu1readchip->setEnabled(true);
        ui->readchipmsg->setEnabled(true);
        ui->msu0filepath->setEnabled(true);
        ui->msu1filepath->setEnabled(true);
        ui->msu0openfile->setEnabled(true);
        ui->msu1openfile->setEnabled(true);
        ui->msu0load->setEnabled(true);
        ui->msu1load->setEnabled(true);
        ui->netctl->setText("启动远程调用");
        disconnect(&udpSocket, 0, 0, 0);
        netctlStatus = 0;
        ui->tips->appendPlainText("关闭远程调用成功。");
    }
}
