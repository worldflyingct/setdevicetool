#include "tkmisp.h"
#include "ui_tkmisp.h"
#include "common/common.h"
#include <QFileDialog>
#include <QThread>

#define ISP_SYNC                0x7f
#define ISP_SYNC_TWO            0x8f
#define ISP_ERASE               0x43
#define ISP_WRITE               0x31
#define ISP_READ                0x08
#define ISP_GETINFO             0x01

#define BTN_STATUS_IDLE             0x00
#define BTN_STATUS_WRITE            0x01
#define BTN_STATUS_READ0            0x02
#define BTN_STATUS_READ1            0x05
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
        retrytime = 0;
        bufflen = 0;
        if (btnStatus == BTN_STATUS_WRITE || btnStatus == BTN_STATUS_ERASE) {
            ui->tips->appendPlainText("串口同步成功，开始擦除操作");
            chipstep = ISP_ERASE;
        } else if (btnStatus == BTN_STATUS_READ0) {
            ui->tips->appendPlainText("串口同步成功，开始读取msu0操作");
            chipstep = ISP_READ;
            char buff[7];
            buff[0] = ISP_READ;
            addr = 0x00020000;
            buff[4] = addr>>24;
            buff[3] = addr>>16;
            buff[2] = addr>>8;
            buff[1] = addr;
            unsigned int len = bin0len > 768 ? 768 : bin0len;
            buff[6] = len >> 8;
            buff[5] = len;
            offset = 0;
            serial.write(buff, 7);
        } else if (btnStatus == BTN_STATUS_READ1) {
            ui->tips->appendPlainText("串口同步成功，开始读取msu1操作");
            chipstep = ISP_READ;
            char buff[7];
            buff[0] = ISP_READ;
            addr = 0x00010000;
            buff[0] = ISP_READ;
            buff[4] = addr>>24;
            buff[3] = addr>>16;
            buff[2] = addr>>8;
            buff[1] = addr;
            unsigned int len = bin1len > 768 ? 768 : bin1len;
            buff[6] = len >> 8;
            buff[5] = len;
            offset = 0;
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
    } else if (chipstep == ISP_READ) { // ISP_READ
        if (serialReadBuff[0] != 0x09) {
            retrytime++;
            if (retrytime == 250) {
                ui->tips->appendPlainText("读取失败");
                CloseSerial();
                return;
            }
            bufflen = 0;
            unsigned int len;
            if (btnStatus == BTN_STATUS_READ0) {
                addr = 0x00020000;
                len = bin0len > 768 ? 768 : bin0len;
            } else if (btnStatus == BTN_STATUS_READ1) {
                addr = 0x00010000;
                len = bin1len > 768 ? 768 : bin1len;
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
        } else {
            if (bufflen >= 7) {
                unsigned char address[4];
                address[3] = addr>>24;
                address[2] = addr>>16;
                address[1] = addr>>8;
                address[0] = addr;
                if (!memcmp(address, serialReadBuff+1, 4)) {
                    unsigned int len = 256*serialReadBuff[6] + serialReadBuff[5];
                    if (bufflen == len + 7) { // 获取数据完毕
                        unsigned char *bin;
                        unsigned int partitiononesize;
                        unsigned int partitiontwoposition;
                        if (btnStatus == BTN_STATUS_READ0) {
                            bin = bin0;
                            partitiononesize = 128*1024-4;
                            partitiontwoposition = 0x48000;
                        } else if (btnStatus == BTN_STATUS_READ1) {
                            bin = bin1;
                            partitiononesize = 64*1024-4;
                            partitiontwoposition = 0x40000;
                        } else {
                            ui->tips->appendPlainText("程序内部错误");
                            CloseSerial();
                            return;
                        }
                        memcpy(bin+offset, serialReadBuff+7, len);
                        addr += len;
                        offset += len;
                        if (addr == partitiononesize) {
                            addr = 0x48000;
                        }
                        if (bin0len < partitiononesize || addr >= partitiontwoposition) {
                            if (bin0len-offset < 768) {
                                len = bin0len-offset;
                            } else {
                                len = 768;
                            }
                        } else {
                            if (partitiononesize - offset < 768) {
                                len = 128*1024 - 4 - offset;
                            } else {
                                len = 768;
                            }
                        }
                        if (len == 0) {
                            // 这里将文件写入
                            char buff[64];
                            if (COMMON::filewrite(savefilepath, (char*)bin, offset, buff, 0)) {
                                ui->tips->appendPlainText(buff);
                                CloseSerial();
                            }
                            sprintf(buff, "全部读出完成，一共读出%uk数据", offset / 1024);
                            ui->tips->appendPlainText(buff);
                            CloseSerial();
                            return;
                        } else {
                            bufflen = 0;
                            char buff[64];
                            qDebug("offset:%d", offset);
                            sprintf(buff, "已读出%uk数据", offset / 1024);
                            ui->tips->appendPlainText(buff);
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
                    ui->tips->appendPlainText("读取失败");
                    CloseSerial();
                    return;
                }
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
    if (chipstep == ISP_ERASE) {
        timer.start(20000); // 正在擦除，所需时间比较长
    } else if (chipstep == ISP_SYNC) {
        timer.start(1000); // 正在同步，所需时间比较短
    } else {
        timer.start(3000); // 非ISP_SYNC命令，非擦除命令，统一等待3s
    }
}

int TkmIsp::OpenSerial () {
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
    bool notfoundload = true;
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
        unsigned char *chardata = (unsigned char*)bytedata.data();
        if (!suffix.compare(".hex")) {
            int res = COMMON::TKM_HexToBin(chardata, bytedata.length(), bin0, sizeof(bin0), &bin0len);
            if (res == -1) {
                ui->tips->appendPlainText("hex文件格式错误");
                return;
            } else if (res == -2) {
                ui->tips->appendPlainText("输出空间太小");
                return;
            }
        } else if (!suffix.compare(".bin")) {
            uint len = bytedata.length();
            if (len > sizeof(bin0)) {
                ui->tips->appendPlainText("bin文件太大");
                return;
            }
            memcpy(bin0, chardata, len);
            bin0len = len;
        } else {
            ui->tips->appendPlainText("msu0文件类型错误");
            return;
        }
        notfoundload = false;
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
        unsigned char *chardata = (unsigned char*)bytedata.data();
        if (!suffix.compare(".hex")) {
            int res = COMMON::TKM_HexToBin(chardata, bytedata.length(), bin1, sizeof(bin1), &bin1len);
            if (res == -1) {
                ui->tips->appendPlainText("hex文件格式错误");
                return;
            } else if (res == -2) {
                ui->tips->appendPlainText("输出空间太小");
                return;
            }
        } else if (!suffix.compare(".bin")) {
            uint len = bytedata.length();
            if (len > sizeof(bin1)) {
                ui->tips->appendPlainText("bin文件太大");
                return;
            }
            memcpy(bin1, chardata, len);
            bin1len = len;
        } else {
            ui->tips->appendPlainText("msu0文件类型错误");
            return;
        }
        notfoundload = false;
    }
    if (notfoundload) {
        ui->tips->appendPlainText("没有需要编译的分区");
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
    btnStatus = BTN_STATUS_WRITE;
    sprintf(buff, "等待设备连接...%u", ++retrytime);
    ui->tips->appendPlainText(buff);
}

void TkmIsp::on_erasechip_clicked () {
}

void TkmIsp::on_clearlog_clicked () {
}

void TkmIsp::on_msu0readchip_clicked () {
    if (btnStatus) {
        ui->tips->appendPlainText("用户终止操作");
        CloseSerial();
        return;
    }
    sscanf(ui->flashsize->currentText().toStdString().c_str(), "%u", &bin0len);
    bin0len *= 1024;
    QString filepath = QFileDialog::getSaveFileName(this, "保存镜像", NULL, "镜像文件(*.bin)");
    if (!filepath.length()) {
        return;
    }
    savefilepath = filepath;
    retrytime = 0;
    chipstep = ISP_SYNC; // sync
    char buff[64];
    buff[0] = ISP_SYNC;
    if (OpenSerial()) {
        ui->tips->appendPlainText("串口打开失败");
        return;
    }
    btnStatus = BTN_STATUS_READ0;
    sprintf(buff, "等待设备连接...%u", ++retrytime);
    ui->tips->appendPlainText(buff);
}

void TkmIsp::on_msu1readchip_clicked() {
    if (btnStatus) {
        ui->tips->appendPlainText("用户终止操作");
        CloseSerial();
        return;
    }
    sscanf(ui->flashsize->currentText().toStdString().c_str(), "%u", &bin1len);
    if (bin1len > 96) {
        ui->tips->appendPlainText("msu1文件太大");
        CloseSerial();
        return;
    }
    bin1len *= 1024;
    QString filepath = QFileDialog::getSaveFileName(this, "保存镜像", NULL, "镜像文件(*.bin)");
    if (!filepath.length()) {
        return;
    }
    savefilepath = filepath;
    retrytime = 0;
    chipstep = ISP_SYNC; // sync
    char buff[64];
    buff[0] = ISP_SYNC;
    if (OpenSerial()) {
        ui->tips->appendPlainText("串口打开失败");
        return;
    }
    btnStatus = BTN_STATUS_READ1;
    sprintf(buff, "等待设备连接...%u", ++retrytime);
    ui->tips->appendPlainText(buff);
}


void TkmIsp::on_readchipmsg_clicked() {
}

