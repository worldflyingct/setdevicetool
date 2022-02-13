#include "stm32isp.h"
#include "ui_stm32isp.h"
#include <QMessageBox>
// 公共函数库
#include "common/common.h"
#include "common/hextobin.h"

#define ISP_SYNC          0x7f
#define ISP_ERASE         0x43
#define ISP_ERASE_TWO     0x44
#define ISP_WRITE         0x31
#define ISP_WRITE_TWO     0x32
#define ISP_WRITE_THREE   0x33
#define ISP_WRITE_FOUR    0x34
#define ISP_READ          0x11
#define ISP_READ_TWO      0x12
#define ISP_READ_THREE    0x13
#define ISP_READ_FOUR     0x14

Stm32Isp::Stm32Isp(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Stm32Isp) {
    ui->setupUi(this);
    GetComList();
}

Stm32Isp::~Stm32Isp() {
    delete ui;
}

// 获取可用的串口号
void Stm32Isp::GetComList () {
    QComboBox *c = ui->com;
    for (int a = c->count() ; a > 0 ; a--) {
        c->removeItem(a-1);
    }
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        c->addItem(info.portName() + " " + info.description());
    }
}

void Stm32Isp::on_refresh_clicked() {
    GetComList();
}

void Stm32Isp::TimerOutEvent() {
    if (btnStatus == 1) {
        ui->tips->appendPlainText("设备响应超时");
    } else if (btnStatus == 2) {
        ui->tips->appendPlainText("数据接收超时");
    }
    CloseSerial();
    btnStatus = 0;
}

void Stm32Isp::ReadSerialData() {
    unsigned int i;
    QByteArray arr = serial.readAll();
    int len = arr.length();
    char *c = arr.data();
    memcpy(serialReadBuff+bufflen, c, len);
    bufflen += len;
    if (chipstep == ISP_SYNC) { // sync
        if (bufflen != 1 || (bufflen == 1 && serialReadBuff[0] != 0x79)) {
            retrytime++;
            if (retrytime == 50) {
                ui->tips->appendPlainText("波特率同步失败");
                CloseSerial();
                btnStatus = 0;
                return;
            }
            bufflen = 0;
            char buff[1];
            buff[0] = chipstep;
            serial.write(buff, 1);
        } else {
            ui->tips->appendPlainText("波特率同步成功");
            retrytime = 0;
            bufflen = 0;
            if (btnStatus == 1) {
                chipstep = ISP_ERASE;
                char buff[2];
                buff[0] = ISP_ERASE;
                buff[1] = 0xbc;
                serial.write(buff, 2);
            } else {
                addr = 0;
                char buff[2];
                chipstep = ISP_READ;
                buff[0] = ISP_READ;
                buff[1] = 0xee;
                serial.write(buff, 2);
            }
        }
    } else if (chipstep == ISP_ERASE || chipstep == ISP_ERASE_TWO) { // erase
        if (bufflen != 1 || (bufflen == 1 && serialReadBuff[0] != 0x79)) {
            retrytime++;
            if (retrytime == 50) {
                ui->tips->appendPlainText("擦除初始化失败");
                CloseSerial();
                btnStatus = 0;
                return;
            }
            bufflen = 0;
            chipstep = ISP_ERASE;
            char buff[2];
            buff[0] = ISP_ERASE;
            buff[1] = 0xbc;
            serial.write(buff, 2);
        } else {
            retrytime = 0;
            bufflen = 0;
            if (btnStatus == 1) {
                if (chipstep == ISP_ERASE) {
                    ui->tips->appendPlainText("擦除初始化成功，开始擦除");
                    chipstep = ISP_ERASE_TWO;
                    char checksum = 0;
                    unsigned int pagenumbers = binlen / 2048 + 1;
                    char buff[pagenumbers+2];
                    buff[0] = pagenumbers - 1;
                    checksum ^= buff[0];
                    for(i = 0 ; i < pagenumbers ; i++) {
                        buff[i+1] = i;
                        checksum ^= buff[i+1];
                    }
                    buff[pagenumbers+1] = checksum;
                    serial.write(buff, pagenumbers+2);
                } else if (chipstep == ISP_ERASE_TWO) {
                    ui->tips->appendPlainText("擦除完毕，开始写入");
                    ui->tips->appendPlainText("已成功写入0k数据");
                    addr = 0;
                    chipstep = ISP_WRITE;
                    char buff[2];
                    buff[0] = ISP_WRITE;
                    buff[1] = 0xce;
                    serial.write(buff, 2);
                } else {
                    ui->tips->appendPlainText("程序内部错误");
                }
            }
        }
    } else if (chipstep == ISP_WRITE || chipstep == ISP_WRITE_TWO || chipstep == ISP_WRITE_THREE || chipstep == ISP_WRITE_FOUR) { // write
        if (bufflen != 1 || (bufflen == 1 && serialReadBuff[0] != 0x79)) {
            retrytime++;
            if (retrytime == 50) {
                ui->tips->appendPlainText("写入失败");
                CloseSerial();
                btnStatus = 0;
                return;
            }
            bufflen = 0;
            chipstep = ISP_WRITE;
            char buff[2];
            buff[0] = ISP_WRITE;
            buff[1] = 0xce;
            serial.write(buff, 2);
        } else {
            retrytime = 0;
            bufflen = 0;
            if (btnStatus == 1) {
                if (chipstep == ISP_WRITE) {
                    chipstep = ISP_WRITE_TWO;
                    char checksum = 0;
                    unsigned long tmp_addr = addr + 0x08000000;
                    char buff[5];
                    buff[0] = tmp_addr >> 24;
                    checksum ^= buff[0];
                    buff[1] = tmp_addr >> 16;
                    checksum ^= buff[1];
                    buff[2] = tmp_addr >> 8;
                    checksum ^= buff[2];
                    buff[3] = tmp_addr;
                    checksum ^= buff[3];
                    buff[4] = checksum;
                    serial.write(buff, 5);
                } else if (chipstep == ISP_WRITE_TWO) {
                    if (binlen - addr > 256) {
                        chipstep = ISP_WRITE_THREE;
                        char checksum = 0;
                        char buff[258];
                        buff[0] = 0xff;
                        checksum ^= buff[0];
                        for (i = 0 ; i < 256 ; i++) {
                            buff[i+1] = bin[addr+i];
                            checksum ^= buff[i+1];
                        }
                        buff[257] = checksum;
                        serial.write(buff, 258);
                    } else { // 最后的一次写入
                        chipstep = ISP_WRITE_FOUR;
                        char checksum = 0;
                        unsigned int len = binlen-addr;
                        char buff[len+2];
                        buff[0] = len-1;
                        checksum ^= buff[0];
                        for (i = 0 ; i < len ; i++) {
                            buff[i+1] = bin[addr+i];
                            checksum ^= buff[i+1];
                        }
                        buff[len+1] = checksum;
                        serial.write(buff, len+2);
                    }
                } else if (chipstep == ISP_WRITE_THREE) {
                    addr += 256;
                    char buff[64];
                    if (addr % 2048 == 0) {
                        QTextCursor pos = ui->tips->textCursor();
                        pos.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
                        pos.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
                        pos.removeSelectedText();
                        pos.deletePreviousChar();
                        sprintf(buff, "已成功写入%luk数据", addr / 1024);
                        ui->tips->appendPlainText(buff);
                    }
                    chipstep = ISP_WRITE;
                    buff[0] = ISP_WRITE;
                    buff[1] = 0xce;
                    serial.write(buff, 2);
                } else if (chipstep == ISP_WRITE_FOUR) {
                    if (ui->checkwrite->isChecked()) {
                        char buff[64];
                        sprintf(buff, "全部写入完成，一共写入%luk数据，开始校验", addr / 1024);
                        ui->tips->appendPlainText(buff);
                        ui->tips->appendPlainText("已校验0k数据");
                        addr = 0;
                        chipstep = ISP_READ;
                        buff[0] = ISP_READ;
                        buff[1] = 0xee;
                        serial.write(buff, 2);
                    } else {
                        char buff[64];
                        sprintf(buff, "全部写入完成，一共写入%luk数据", addr / 1024);
                        ui->tips->appendPlainText(buff);
                        CloseSerial();
                        btnStatus = 0;
                    }
                } else {
                    ui->tips->appendPlainText("程序内部错误");
                }
            }
        }
    } else if (chipstep == ISP_READ || chipstep == ISP_READ_TWO) { // read
        if (bufflen != 1 || (bufflen == 1 && serialReadBuff[0] != 0x79)) {
            retrytime++;
            if (retrytime == 50) {
                ui->tips->appendPlainText("读取失败");
                CloseSerial();
                btnStatus = 0;
                return;
            }
            bufflen = 0;
            chipstep = ISP_READ;
            char buff[2];
            buff[0] = ISP_READ;
            buff[1] = 0xee;
            serial.write(buff, 2);
        } else {
            retrytime = 0;
            bufflen = 0;
            if (chipstep == ISP_READ) {
                chipstep = ISP_READ_TWO;
                char checksum = 0;
                unsigned long tmp_addr = addr + 0x08000000;
                char buff[5];
                buff[0] = tmp_addr >> 24;
                checksum ^= buff[0];
                buff[1] = tmp_addr >> 16;
                checksum ^= buff[1];
                buff[2] = tmp_addr >> 8;
                checksum ^= buff[2];
                buff[3] = tmp_addr;
                checksum ^= buff[3];
                buff[4] = checksum;
                serial.write(buff, 5);
            } else if (chipstep == ISP_READ_TWO) {
                if (binlen - addr > 256) {
                    chipstep = ISP_READ_THREE;
                    char buff[2];
                    buff[0] = 0xff;
                    buff[1] = ~buff[0];
                    serial.write(buff, 2);
                } else {
                    chipstep = ISP_READ_FOUR;
                    int len = binlen-addr;
                    char buff[2];
                    buff[0] = len-1;
                    buff[1] = ~buff[0];
                    serial.write(buff, 2);
                }
            }
        }
    } else if (chipstep == ISP_READ_THREE && bufflen == 257) { // read
        if (serialReadBuff[0] != 0x79) {
            retrytime++;
            if (retrytime == 50) {
                ui->tips->appendPlainText("读取失败");
                CloseSerial();
                btnStatus = 0;
                return;
            }
            bufflen = 0;
            chipstep = ISP_READ;
            char buff[2];
            buff[0] = ISP_READ;
            buff[1] = 0xee;
            serial.write(buff, 2);
        } else {
            retrytime = 0;
            bufflen = 0;
            char buff[64];
            for (i = 0 ; i < 256 ; i++) {
                if (bin[addr+i] != serialReadBuff[i+1]) {
                    sprintf(buff, "校验失败, addr:%lu,i:%u,bin:0x%02x,buff:0x%02x", addr, i, bin[addr+i], serialReadBuff[i+1]);
                    ui->tips->appendPlainText(buff);
                    CloseSerial();
                    btnStatus = 0;
                    return;
                }
            }
            addr += 256;
            if (addr % 2048 == 0) {
                QTextCursor pos = ui->tips->textCursor();
                pos.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
                pos.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
                pos.removeSelectedText();
                pos.deletePreviousChar();
                sprintf(buff, "已校验%luk数据", addr / 1024);
                ui->tips->appendPlainText(buff);
            }
            chipstep = ISP_READ;
            buff[0] = ISP_READ;
            buff[1] = 0xee;
            serial.write(buff, 2);
        }
    } else if (chipstep == ISP_READ_FOUR && bufflen == binlen-addr+1) { // read
        if (serialReadBuff[0] != 0x79) {
            retrytime++;
            if (retrytime == 50) {
                ui->tips->appendPlainText("读取失败");
                CloseSerial();
                btnStatus = 0;
                return;
            }
            bufflen = 0;
            chipstep = ISP_READ;
            char buff[2];
            buff[0] = ISP_READ;
            buff[1] = 0xee;
            serial.write(buff, 2);
        } else {
            retrytime = 0;
            bufflen = 0;
            char buff[64];
            for (i = 0 ; i < binlen-addr ; i++) {
                if (bin[addr+i] != serialReadBuff[i+1]) {
                    sprintf(buff, "校验失败, addr:%lu,i:%u,bin:0x%02x,buff:0x%02x", addr, i, bin[addr+i], serialReadBuff[i+1]);
                    ui->tips->appendPlainText(buff);
                    CloseSerial();
                    btnStatus = 0;
                    return;
                }
            }
            sprintf(buff, "全部校验完成，一共校验%luk数据", addr / 1024);
            ui->tips->appendPlainText(buff);
            CloseSerial();
            btnStatus = 0;
        }
    } else if (chipstep != 0x13 && chipstep != 0x14){
        char buff[64];
        sprintf(buff, "chipstep:0x%02x, bufflen:%d, ack:0x%02x", chipstep, bufflen, serialReadBuff[0]);
        ui->tips->appendPlainText(buff);
    }
    timer.stop();
    timer.start(8000);
}

int Stm32Isp::OpenSerial(char *data, qint64 len) {
    char comname[12];
    sscanf(ui->com->currentText().toStdString().c_str(), "%s ", comname);
    serial.setPortName(comname);
    serial.setBaudRate(QSerialPort::Baud115200);
    serial.setParity(QSerialPort::EvenParity);
    serial.setDataBits(QSerialPort::Data8);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::NoFlowControl);
    if (!serial.open(QIODevice::ReadWrite)) {
        return -1;
    }
    connect(&timer, SIGNAL(timeout()), this, SLOT(TimerOutEvent()));
    connect(&serial, SIGNAL(readyRead()), this, SLOT(ReadSerialData()));
    timer.start(8000);
    serial.write(data, len);
    return 0;
}

void Stm32Isp::CloseSerial() {
    disconnect(&timer, 0, 0, 0);
    timer.stop();
    disconnect(&serial, 0, 0, 0);
    serial.close();
    bufflen = 0;
}

void Stm32Isp::on_openfile_clicked() {
    QString filepath = QFileDialog::getOpenFileName(this, "选择镜像", NULL, "镜像文件(*.hex *.bin)");
    if (filepath.length()) {
        ui->filepath->setText(filepath);
    }
}

void Stm32Isp::on_readchip_clicked() {
    QString filepath = QFileDialog::getSaveFileName(this, "保存镜像", NULL, "镜像文件(*.bin)");
    if (filepath.length()) {
        btnStatus = 2;
        chipstep = 0x7f; // sync
        retrytime = 0;
        char buff[1];
        buff[0] = chipstep;
        if (OpenSerial(buff, 1)) {
            ui->tips->appendPlainText("串口打开失败");
            return;
        }
        ui->tips->appendPlainText("开始同步串口");
    }
}

void Stm32Isp::on_readchipmsg_clicked() {
}

void Stm32Isp::on_writechip_clicked() {
    QString filepath = ui->filepath->text();
    int pos = filepath.lastIndexOf(".");
    QString suffix = filepath.mid(pos);
    QFile file(filepath);
    if(!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, "错误", "无法读取烧录文件");
        return;
    }
    QByteArray bytedata = file.readAll();
    file.close();
    unsigned char *chardata = (unsigned char*)bytedata.data();
    if (!suffix.compare(".hex")) {
        if (HexToBin(chardata, bytedata.length(), bin, sizeof(bin), 0x08000000, &binlen) != RES_OK) {
            QMessageBox::information(this, "错误", "hex文件格式错误");
            return;
        }
    } else if (!suffix.compare(".bin")) {
        uint len = bytedata.length();
        if (len > sizeof(bin)) {
            QMessageBox::information(this, "错误", "bin文件太大");
            return;
        }
        memcpy(bin, chardata, len);
        binlen = len;
    } else {
        QMessageBox::information(this, "错误", "文件类型错误");
        return;
    }
/*
    while (binlen % 4 != 0) {
        bin[binlen] = 0xff;
        binlen++;
    }
*/
    btnStatus = 1;
    chipstep = 0x7f; // sync
    retrytime = 0;
    char buff[1];
    buff[0] = chipstep;
    if (OpenSerial(buff, 1)) {
        ui->tips->appendPlainText("串口打开失败");
        return;
    }
    ui->tips->appendPlainText("开始同步串口");
    return;
}

void Stm32Isp::on_clearlog_clicked() {
    ui->tips->clear();
}
