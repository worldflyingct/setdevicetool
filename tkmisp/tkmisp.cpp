#include "tkmisp.h"
#include "ui_tkmisp.h"
#include <QFileDialog>
#include "common/hextobin.h"
#include "common/common.h"

#define ISP_SYNC                0x7f
#define ISP_SYNC_TWO            0x8f
#define ISP_CHECK               0x9f
#define ISP_ERASE               0x0c
#define ISP_ERASE_ACK           0x0d
#define ISP_WRITE               0x02
#define ISP_WRITE_ACK           0x03
#define ISP_READ                0x08
#define ISP_READ_ACK            0x09
#define ISP_GETINFO             0x00
#define ISP_GETINFO_ACK         0x01

#define BTN_STATUS_IDLE             0x00
#define BTN_STATUS_WRITE            0x01
#define BTN_STATUS_READ             0x02
#define BTN_STATUS_ERASE            0x03
#define BTN_STATUS_READ_MSG         0x04

TkmIsp::TkmIsp(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TkmIsp) {
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
    } else {
        ui->tips->appendPlainText("设备响应超时");
    }
    CloseSerial();
}

void TkmIsp::SerialErrorEvent () {
    ui->tips->appendPlainText("串口错误");
    ui->tips->appendPlainText("");
    CloseSerial();
    GetComList();
}

int TkmIsp::GetBtnStatus () {
    return btnStatus;
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
        if (btnStatus == BTN_STATUS_READ_MSG) {
            ui->tips->appendPlainText("串口同步成功，开始获取芯片信息");
            chipstep = ISP_GETINFO;
            char buff[7];
            buff[0] = ISP_GETINFO;
            buff[4] = 0;
            buff[3] = 0;
            buff[2] = 0;
            buff[1] = 0;
            buff[6] = 0;
            buff[5] = 0;
            serial.write(buff, 7);
        } else if (btnStatus == BTN_STATUS_WRITE || btnStatus == BTN_STATUS_ERASE) {
            ui->tips->appendPlainText("串口同步成功，开始擦除操作");
            chipstep = ISP_ERASE;
            char buff[7];
            buff[0] = ISP_ERASE;
            if (btnStatus == BTN_STATUS_ERASE || bootlen > 0) {
                addr = 0x00000000;
            } else if (bin1len > 0) {
                addr = 0x00010000;
            } else if (bin0len > 0) {
                addr = 0x00020000;
            } else {
                ui->tips->appendPlainText("程序内部错误");
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
            uint len;
            if (bootlen > 0) {
                ui->tips->appendPlainText("串口同步成功，开始读取boot操作");
                addr = 0x00000000;
            } else if (bin1len > 0) {
                ui->tips->appendPlainText("串口同步成功，开始读取msu1操作");
                addr = 0x00010000;
                len = bin1len > 0x300 ? 0x300 : bin1len;
            } else if (bin0len > 0) {
                ui->tips->appendPlainText("串口同步成功，开始读取msu0操作");
                addr = 0x00020000;
                len = bin0len > 0x300 ? 0x300 : bin0len;
            } else {
                len = 0;
                ui->tips->appendPlainText("程序内部错误");
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
            ui->tips->appendPlainText("串口同步成功，开始读取芯片信息操作");
            chipstep = ISP_GETINFO;
        } else {
            char buff[64];
            sprintf(buff, "程序内部错误%d", __LINE__);
            ui->tips->appendPlainText(buff);
            CloseSerial();
            return;
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
            buff[4] = 0;
            buff[3] = 0;
            buff[2] = 0;
            buff[1] = 0;
            buff[6] = 0;
            buff[5] = 0;
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
            if (bootlen > 0) {
                addr = 0x00000000;
                len = bootlen > 0x300 ? 0x300 : bootlen;
            } else if (bin1len > 0) {
                addr = 0x00010000;
                len = bin1len > 0x300 ? 0x300 : bin1len;
            } else if (bin0len > 0) {
                addr = 0x00020000;
                len = bin0len > 0x300 ? 0x300 : bin0len;
            } else {
                ui->tips->appendPlainText("程序内部错误");
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
                if (addr < 0x10000) {
                    bin = boot;
                    binlen = bootlen;
                    partitionOneSize = 0x10000;
                    partitionTwoStartPosition = 0x10000;
                } else if ((0x10000 <= addr && addr < 0x20000) || (0x40000 <= addr && addr < 0x48000)) {
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
                    ui->tips->appendPlainText("程序内部错误");
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
                return;
            }
            if (btnStatus == BTN_STATUS_ERASE || bootlen > 0) {
                addr = 0x00000000;
            } else if (bin1len > 0) {
                addr = 0x00010000;
            } else if (bin0len > 0) {
                addr = 0x00020000;
            } else {
                ui->tips->appendPlainText("程序内部错误");
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
                return;
            }
            char buff[128];
            uchar *bin;
            uint binlen;
            uint partitionOneEndPosition;
            uint partitionTwoStartPosition;
            if (btnStatus == BTN_STATUS_ERASE) {
                bin = NULL;
                binlen = 0x78000;
                partitionOneEndPosition = 0x78000;
                partitionTwoStartPosition = 0x78000;
            } else if (addr < 0x10000) { // boot
                bin = boot;
                binlen = bootlen;
                partitionOneEndPosition = 0x10000;
                partitionTwoStartPosition = 0x10000;
            } else if ((0x10000 <= addr && addr < 0x20000) || (0x40000 <= addr && addr < 0x48000)) { // msu1
                bin = bin1;
                binlen = bin1len;
                partitionOneEndPosition = 0x20000;
                partitionTwoStartPosition = 0x40000;
            } else if ((0x20000 <= addr && addr < 0x40000) || (0x48000 <= addr && addr < 0x68000)) { // msu0
                bin = bin0;
                binlen = bin0len;
                partitionOneEndPosition = 0x40000;
                partitionTwoStartPosition = 0x48000;
            } else {
                ui->tips->appendPlainText("程序内部错误");
                CloseSerial();
                return;
            }
            offset += 0x1000;
            QTextCursor pos = ui->tips->textCursor();
            pos.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
            pos.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            pos.removeSelectedText();
            pos.deletePreviousChar();
            if (offset < binlen) {                
                if (bin == boot) {
                    sprintf(buff, "已擦除boot %uk数据", offset / 1024);
                } else if (bin == bin1) {
                    sprintf(buff, "已擦除msu1 %uk数据", offset / 1024);
                } else if (bin == bin0) {
                    sprintf(buff, "已擦除msu0 %uk数据", offset / 1024);
                } else {
                    sprintf(buff, "已擦除%uk数据", offset / 1024);
                }
                ui->tips->appendPlainText(buff);
                addr += 0x1000;
                if (addr == partitionOneEndPosition) {
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
                return;
            } else if ((bin == boot && (bin1len > 0 || bin0len > 0)) || (bin == bin1 && bin0len > 0)) { // 擦除msu1完毕且需要继续擦除msu0
                if (bin == boot) {
                    sprintf(buff, "擦除boot完毕，一共擦除%uk数据", offset / 1024);
                    ui->tips->appendPlainText(buff);
                    if (bin1len > 0) {
                        ui->tips->appendPlainText("开始擦除msu1");
                        addr = 0x00010000;
                    } else {
                        ui->tips->appendPlainText("开始擦除msu0");
                        addr = 0x00020000;
                    }
                } else {
                    sprintf(buff, "擦除msu1完毕，一共擦除%uk数据", offset / 1024);
                    ui->tips->appendPlainText(buff);
                    ui->tips->appendPlainText("开始擦除msu0");
                    addr = 0x00020000;
                }
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
                if (bin == boot) {
                    sprintf(buff, "擦除boot完毕，一共擦除%uk数据", offset / 1024);
                } else if (bin == bin1) {
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
                uchar *bin;
                if (bootlen > 0) {
                    bin = boot;
                    addr = 0x00000000;
                    len = bootlen - offset > 256 ? 256 : bootlen - offset;
                } else if (bin1len > 0) {
                    bin = bin1;
                    addr = 0x00010000;
                    len = bin1len - offset > 256 ? 256 : bin1len - offset;
                } else if (bin0len > 0) {
                    bin = bin0;
                    addr = 0x00020000;
                    len = bin0len - offset > 256 ? 256 : bin0len - offset;
                } else {
                    ui->tips->appendPlainText("程序内部错误");
                    CloseSerial();
                    return;
                }
                chipstep = ISP_WRITE;
                char *serialWriteBuff = (char*)malloc(len+7);
                if (serialWriteBuff == NULL) {
                    ui->tips->appendPlainText("内存申请失败");
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
        }
    } else if (chipstep == ISP_WRITE) { // ISP_WRITE
        if (serialReadBuff[0] != ISP_WRITE_ACK) {
            retrytime++;
            if (retrytime == 250) {
                ui->tips->appendPlainText("写入失败");
                CloseSerial();
                return;
            }
            offset = 0;
            bufflen = 0;
            chipstep = ISP_WRITE;
            uint len;
            uchar *bin;
            if (bootlen > 0) {
                bin = boot;
                addr = 0x00000000;
                len = bootlen - offset > 256 ? 256 : bootlen - offset;
            } else if (bin1len > 0) {
                bin = bin1;
                addr = 0x00010000;
                len = bin1len - offset > 256 ? 256 : bin1len - offset;
            } else if (bin0len > 0) {
                bin = bin0;
                addr = 0x00020000;
                len = bin0len - offset > 256 ? 256 : bin0len - offset;
            } else {
                ui->tips->appendPlainText("程序内部错误");
                CloseSerial();
                return;
            }
            char *serialWriteBuff = (char*)malloc(len+7);
            if (serialWriteBuff == NULL) {
                ui->tips->appendPlainText("内存申请失败");
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
                return;
            }
            uint len = 256*serialReadBuff[6] + serialReadBuff[5];
            uchar *bin;
            uint binlen;
            uint partitionOneSize;
            uint partitionOneEndPosition;
            uint partitionTwoStartPosition;
            if (addr < 0x10000) { // boot
                bin = boot;
                binlen = bootlen;
                partitionOneSize = 0x10000;
                partitionOneEndPosition = 0x10000;
                partitionTwoStartPosition = 0x10000;
            } else if ((0x10000 <= addr && addr < 0x20000) || (0x40000 <= addr && addr < 0x48000)) { // msu0
                bin = bin1;
                binlen = bin1len;
                partitionOneSize = 0x10000;
                partitionOneEndPosition = 0x20000;
                partitionTwoStartPosition = 0x40000;
            } else if ((0x20000 <= addr && addr < 0x40000) || (0x48000 <= addr && addr < 0x68000)) { // msu0
                bin = bin0;
                binlen = bin0len;
                partitionOneSize = 0x20000;
                partitionOneEndPosition = 0x40000;
                partitionTwoStartPosition = 0x48000;
            } else {
                ui->tips->appendPlainText("程序内部错误");
                CloseSerial();
                return;
            }
            offset += len;
            addr += len;
            if (addr == partitionOneEndPosition) {
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
                if ((bin == boot && (bin1len > 0 || bin0len > 0)) || (bin == bin1 && bin0len > 0)) { // 写入msu1完毕且需要继续写入msu0
                    if (bin == boot) {
                        sprintf(buff, "写入boot完毕，一共写入%uk数据", offset / 1024);
                        ui->tips->appendPlainText(buff);
                        if (bin1len > 0) {
                            ui->tips->appendPlainText("开始写入msu1");
                            bin = bin1;
                            addr = 0x00010000;
                            len = bin1len;
                        } else {
                            ui->tips->appendPlainText("开始写入msu0");
                            bin = bin0;
                            addr = 0x00020000;
                            len = bin0len;
                        }
                    } else {
                        sprintf(buff, "写入msu1完毕，一共写入%uk数据", offset / 1024);
                        ui->tips->appendPlainText(buff);
                        ui->tips->appendPlainText("开始写入msu0");
                        bin = bin0;
                        addr = 0x00020000;
                        len = bin0len;
                    }
                    offset = 0;
                    len = len - offset > 256 ? 256 : len - offset;
                } else {
                    if (bin == boot) {
                        sprintf(buff, "写入boot完毕，一共写入%uk数据", offset / 1024);
                    } else if (bin == bin1) {
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
                        return;
                    }
                }
            } else {
                if (bin == boot) {
                    sprintf(buff, "已写入boot %uk数据", offset / 1024);
                } else if (bin == bin1) {
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
                uint len;
                if (bootlen > 0) {
                    addr = 0x00000000;
                    len = bootlen > 0x300 ? 0x300 : bootlen;
                } else if (bin1len > 0) {
                    addr = 0x00010000;
                    len = bin1len > 0x300 ? 0x300 : bin1len;
                } else if (bin0len > 0) {
                    addr = 0x00020000;
                    len = bin0len > 0x300 ? 0x300 : bin0len;
                } else {
                    ui->tips->appendPlainText("程序内部错误");
                    CloseSerial();
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
                return;
            }
            offset = 0;
            bufflen = 0;
            uint len;
            if (bootlen > 0) {
                addr = 0x00000000;
                len = bootlen > 0x300 ? 0x300 : bootlen;
            } else if (bin1len > 0) {
                addr = 0x00010000;
                len = bin1len > 0x300 ? 0x300 : bin1len;
            } else if (bin0len > 0) {
                addr = 0x00020000;
                len = bin0len > 0x300 ? 0x300 : bin0len;
            } else {
                ui->tips->appendPlainText("程序内部错误");
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
                ui->tips->appendPlainText("校验失败");
                CloseSerial();
                return;
            }
            uint len = 256*serialReadBuff[6] + serialReadBuff[5];
            if (bufflen == len + 7) { // 获取数据完毕
                uchar *bin;
                uint binlen;
                uint partitionOneSize;
                uint partitionTwoStartPosition;
                if (addr < 0x10000) {
                    bin = boot;
                    binlen = bootlen;
                    partitionOneSize = 0x10000;
                    partitionTwoStartPosition = 0x10000;
                } else if ((0x10000 <= addr && addr < 0x20000) || (0x40000 <= addr && addr < 0x48000)) {
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
                    ui->tips->appendPlainText("程序内部错误");
                    CloseSerial();
                    return;
                }
                if (memcmp(bin+offset, serialReadBuff+7, len)) {
                    ui->tips->appendPlainText("程序校验错误");
                    CloseSerial();
                    return;
                }
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
                char buff[64];
                QTextCursor pos = ui->tips->textCursor();
                pos.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
                pos.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
                pos.removeSelectedText();
                pos.deletePreviousChar();
                if (len == 0) {
                    if ((bin == boot && (bin1len > 0 || bin0len > 0)) || (bin == bin1 && bin0len > 0)) {
                        if (bin == boot) {
                            sprintf(buff, "校验boot完毕，一共校验%uk数据", offset / 1024);
                            ui->tips->appendPlainText(buff);
                            if (bin1len > 0) {
                                ui->tips->appendPlainText("开始校验msu1");
                                bin = bin1;
                                addr = 0x00010000;
                                len = bin1len;
                            } else {
                                ui->tips->appendPlainText("开始校验msu0");
                                bin = bin0;
                                addr = 0x00020000;
                                len = bin0len;
                            }
                        } else {
                            sprintf(buff, "校验msu1完毕，一共校验%uk数据", offset / 1024);
                            ui->tips->appendPlainText(buff);
                            ui->tips->appendPlainText("开始校验msu0");
                            bin = bin0;
                            addr = 0x00020000;
                            len = bin0len;
                        }
                        offset = 0;
                        len = len - offset > 256 ? 256 : len - offset;
                    } else {
                        if (bin == boot) {
                            sprintf(buff, "校验boot完毕，一共校验%uk数据", offset / 1024);
                        } else if (bin == bin1) {
                            sprintf(buff, "校验msu1完毕，一共校验%uk数据", offset / 1024);
                        } else if (bin == bin0) {
                            sprintf(buff, "校验msu0完毕，一共校验%uk数据", offset / 1024);
                        } else {
                            sprintf(buff, "校验完毕，共校验%uk数据", offset / 1024);
                        }
                        ui->tips->appendPlainText(buff);
                        CloseSerial();
                        return;
                    }
                } else {
                    if (bin == boot) {
                        sprintf(buff, "已校验boot %uk数据", offset / 1024);
                    } else if (bin == bin1) {
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

void TkmIsp::on_bootopenfile_clicked() {
    if (btnStatus) {
        ui->tips->appendPlainText("用户终止操作");
        CloseSerial();
        return;
    }
    QString filepath = QFileDialog::getOpenFileName(this, "选择镜像", NULL, "镜像文件(*.hex *.bin)");
    if (filepath.length()) {
        ui->bootfilepath->setText(filepath);
        ui->bootload->setChecked(true);
    }
}

void TkmIsp::on_msu0openfile_clicked() {
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

void TkmIsp::on_writechip_clicked () {
    if (btnStatus) {
        ui->tips->appendPlainText("用户终止操作");
        CloseSerial();
        return;
    }
    needcheck = ui->checkwrite->isChecked();
    char buff[64];
    bootlen = 0;
    bin0len = 0;
    bin1len = 0;
    if (ui->bootload->isChecked()) {
        QString filepath = ui->bootfilepath->text();
        QFile file(filepath);
        if(!file.open(QIODevice::ReadOnly)) {
            ui->tips->appendPlainText("无法读取烧录文件");
            return;
        }
        QByteArray bytedata = file.readAll();
        file.close();
        int pos = filepath.lastIndexOf(".");
        QString suffix = filepath.mid(pos);
        uchar *chardata = (uchar*)bytedata.data();
        if (!suffix.compare(".hex")) {
            if (chardata[0] == ':') { // 标准ihex文件
                if (HexToBin(chardata, bytedata.length(), boot, sizeof(boot), 0x00000000, &bootlen) != RES_OK) {
                    ui->tips->appendPlainText("hex文件格式错误");
                    return;
                }
            } else { // 本工程独有的hex文件
                int res = COMMON::TKM_HexToBin(chardata, bytedata.length(), boot, sizeof(boot), &bootlen);
                if (res == -1) {
                    ui->tips->appendPlainText("hex文件格式错误");
                    return;
                } else if (res == -2) {
                    sprintf(buff, "镜像文件太大，镜像不能大于%ukbytes", (uint)sizeof(boot)/1024);
                    ui->tips->appendPlainText(buff);
                    return;
                }
            }
        } else if (!suffix.compare(".bin")) {
            uint len = bytedata.length();
            if (len > sizeof(boot)) {
                sprintf(buff, "镜像文件太大，镜像不能大于%ukbytes", (uint)sizeof(boot)/1024);
                ui->tips->appendPlainText(buff);
                return;
            }
            memcpy(boot, chardata, len);
            bootlen = len;
        } else {
            ui->tips->appendPlainText("boot文件类型错误");
            return;
        }
    }
    if (ui->msu0load->isChecked()) {
        QString filepath = ui->msu0filepath->text();
        QFile file(filepath);
        if(!file.open(QIODevice::ReadOnly)) {
            ui->tips->appendPlainText("无法读取烧录文件");
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
                    return;
                }
            } else { // 本工程独有的hex文件
                int res = COMMON::TKM_HexToBin(chardata, bytedata.length(), bin0, sizeof(bin0), &bin0len);
                if (res == -1) {
                    ui->tips->appendPlainText("hex文件格式错误");
                    return;
                } else if (res == -2) {
                    sprintf(buff, "镜像文件太大，镜像不能大于%ukbytes", (uint)sizeof(bin0)/1024);
                    ui->tips->appendPlainText(buff);
                    return;
                }
            }
        } else if (!suffix.compare(".bin")) {
            uint len = bytedata.length();
            if (len > sizeof(bin0)) {
                sprintf(buff, "镜像文件太大，镜像不能大于%ukbytes", (uint)sizeof(bin0)/1024);
                ui->tips->appendPlainText(buff);
                return;
            }
            memcpy(bin0, chardata, len);
            bin0len = len;
        } else {
            ui->tips->appendPlainText("msu0文件类型错误");
            return;
        }
    }
    if (ui->msu1load->isChecked()) {
        QString filepath = ui->msu1filepath->text();
        QFile file(filepath);
        if(!file.open(QIODevice::ReadOnly)) {
            ui->tips->appendPlainText("无法读取烧录文件");
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
                    return;
                }
            } else { // 本工程独有的hex文件
                int res = COMMON::TKM_HexToBin(chardata, bytedata.length(), bin1, sizeof(bin1), &bin1len);
                if (res == -1) {
                    ui->tips->appendPlainText("hex文件格式错误");
                    return;
                } else if (res == -2) {
                    sprintf(buff, "镜像文件太大，镜像不能大于%ukbytes", (uint)sizeof(bin1)/1024);
                    ui->tips->appendPlainText(buff);
                    return;
                }
            }
        } else if (!suffix.compare(".bin")) {
            uint len = bytedata.length();
            if (len > sizeof(bin1)) {
                sprintf(buff, "镜像文件太大，镜像不能大于%ukbytes", (uint)sizeof(bin1)/1024);
                ui->tips->appendPlainText(buff);
                return;
            }
            memcpy(bin1, chardata, len);
            bin1len = len;
        } else {
            ui->tips->appendPlainText("msu0文件类型错误");
            return;
        }
    }
    if (bootlen == 0 && bin0len == 0 && bin1len == 0) {
        ui->tips->appendPlainText("没有需要编译的分区");
        return;
    }
    retrytime = 0;
    chipstep = ISP_SYNC; // sync
    buff[0] = ISP_SYNC;
    if (OpenSerial()) {
        ui->tips->appendPlainText("串口打开失败");
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
    retrytime = 0;
    chipstep = ISP_SYNC; // sync
    char buff[64];
    buff[0] = ISP_SYNC;
    if (OpenSerial()) {
        ui->tips->appendPlainText("串口打开失败");
        return;
    }
    btnStatus = BTN_STATUS_ERASE;
    sprintf(buff, "等待设备连接...%u", ++retrytime);
    ui->tips->appendPlainText(buff);
}

void TkmIsp::on_clearlog_clicked () {
    ui->tips->clear();
}

void TkmIsp::on_bootreadchip_clicked() {
    if (btnStatus) {
        ui->tips->appendPlainText("用户终止操作");
        CloseSerial();
        return;
    }
    char buff[64];
    sscanf(ui->flashsize->text().toUtf8().data(), "%u", &bootlen);
    bootlen *= 1024;
    if (bootlen > sizeof(boot)) {
        sprintf(buff, "计划读取文件太大，可读取的最大值为%ukbytes", (uint)sizeof(boot)/1024);
        ui->tips->appendPlainText(buff);
        CloseSerial();
        return;
    }
    bin0len = 0;
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

void TkmIsp::on_msu0readchip_clicked () {
    if (btnStatus) {
        ui->tips->appendPlainText("用户终止操作");
        CloseSerial();
        return;
    }
    char buff[64];
    sscanf(ui->flashsize->text().toUtf8().data(), "%u", &bin0len);
    bin0len *= 1024;
    if (bin0len > sizeof(bin0)) {
        sprintf(buff, "计划读取文件太大，可读取的最大值为%ukbytes", (uint)sizeof(bin0)/1024);
        ui->tips->appendPlainText(buff);
        CloseSerial();
        return;
    }
    bootlen = 0;
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
    sscanf(ui->flashsize->text().toUtf8().data(), "%u", &bin1len);
    bin1len *= 1024;
    if (bin1len > sizeof(bin1)) {
        sprintf(buff, "计划读取文件太大，可读取的最大值为%ukbytes", (uint)sizeof(bin1)/1024);
        ui->tips->appendPlainText(buff);
        CloseSerial();
        return;
    }
    bootlen = 0;
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
