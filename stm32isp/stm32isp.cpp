#include "stm32isp.h"
#include "ui_stm32isp.h"
// 公共函数库
#include "common/common.h"
#include "common/hextobin.h"

/* 参考文件
https://github.com/nicekwell/stm32ISP/raw/master/documents/stm32isp%20application%20note.pdf
*/

#define ISP_SYNC                0x7f
#define ISP_ERASE               0x43
#define ISP_ERASE_TWO           0x44
#define ISP_WRITE               0x31
#define ISP_WRITE_TWO           0x32
#define ISP_WRITE_THREE         0x33
#define ISP_WRITE_FOUR          0x34
#define ISP_READ                0x11
#define ISP_READ_TWO            0x12
#define ISP_READ_THREE          0x13
#define ISP_READ_FOUR           0x14
#define ISP_GETINFO             0x01
#define ISP_GETINFO_TWO         0x02
#define ISP_WRITEPROTECT        0x63
#define ISP_WRITEPROTECT_TWO    0x64
#define ISP_WRITEUNPROTECT      0x73
#define ISP_READPROTECT         0x82
#define ISP_READUNPROTECT       0x92

#define BTN_STATUS_IDLE             0x00
#define BTN_STATUS_WRITE            0x01
#define BTN_STATUS_READ             0x02
#define BTN_STATUS_ERASE            0x03
#define BTN_STATUS_READ_MSG         0x04
#define BTN_STATUS_WRITE_UNPROTECT  0x05
#define BTN_STATUS_READ_UNPROTECT   0x06
#define BTN_STATUS_WRITE_PROTECT    0x07
#define BTN_STATUS_READ_PROTECT     0x08

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
    ui->tips->appendPlainText("设备响应超时");
    CloseSerial();
    btnStatus = BTN_STATUS_IDLE;
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
                btnStatus = BTN_STATUS_IDLE;
                return;
            }
            bufflen = 0;
            char buff[1];
            buff[0] = ISP_SYNC;
            serial.write(buff, 1);
        } else {
            retrytime = 0;
            bufflen = 0;
            issynced = 1;
            if (btnStatus == BTN_STATUS_WRITE || btnStatus == BTN_STATUS_ERASE) {
                ui->tips->appendPlainText("波特率同步成功，开始擦除操作");
                chipstep = ISP_ERASE;
                char buff[2];
                buff[0] = ISP_ERASE;
                buff[1] = ~buff[0];
                serial.write(buff, 2);
            } else if (btnStatus == BTN_STATUS_READ) {
                ui->tips->appendPlainText("波特率同步成功，开始读取操作");
                addr = 0;
                chipstep = ISP_READ;
                char buff[2];
                buff[0] = ISP_READ;
                buff[1] = ~buff[0];
                serial.write(buff, 2);
            } else if (btnStatus == BTN_STATUS_READ_MSG) {
                ui->tips->appendPlainText("波特率同步成功，开始读取芯片信息操作");
                addr = 0;
                chipstep = ISP_GETINFO;
                char buff[2];
                buff[0] = ISP_GETINFO;
                buff[1] = ~buff[0];
                serial.write(buff, 2);
            } else if (btnStatus == BTN_STATUS_WRITE_PROTECT) {
                ui->tips->appendPlainText("波特率同步成功，开始添加写保护");
                addr = 0;
                chipstep = ISP_WRITEPROTECT;
                char buff[2];
                buff[0] = ISP_WRITEPROTECT;
                buff[1] = ~buff[0];
                serial.write(buff, 2);
            } else if (btnStatus == BTN_STATUS_WRITE_UNPROTECT) {
                ui->tips->appendPlainText("波特率同步成功，开始解除写保护");
                addr = 0;
                chipstep = ISP_WRITEUNPROTECT;
                char buff[2];
                buff[0] = ISP_WRITEUNPROTECT;
                buff[1] = ~buff[0];
                serial.write(buff, 2);
            } else if (btnStatus == BTN_STATUS_READ_PROTECT) {
                ui->tips->appendPlainText("波特率同步成功，开始添加读保护");
                addr = 0;
                chipstep = ISP_READPROTECT;
                char buff[2];
                buff[0] = ISP_READPROTECT;
                buff[1] = ~buff[0];
                serial.write(buff, 2);
            } else if (btnStatus == BTN_STATUS_READ_UNPROTECT) {
                ui->tips->appendPlainText("波特率同步成功，开始解除读保护");
                addr = 0;
                chipstep = ISP_READUNPROTECT;
                char buff[2];
                buff[0] = ISP_READUNPROTECT;
                buff[1] = ~buff[0];
                serial.write(buff, 2);
            } else {
                ui->tips->appendPlainText("程序内部错误1");
                CloseSerial();
                btnStatus = BTN_STATUS_IDLE;
                return;
            }
        }
    } else if (chipstep == ISP_WRITEPROTECT) {
        if (bufflen != 1 || (bufflen == 1 && serialReadBuff[0] != 0x79)) {
            retrytime++;
            if (retrytime == 50) {
                ui->tips->appendPlainText("擦除初始化失败");
                CloseSerial();
                btnStatus = BTN_STATUS_IDLE;
                return;
            }
            bufflen = 0;
            chipstep = ISP_WRITEPROTECT;
            char buff[2];
            buff[0] = ISP_WRITEPROTECT;
            buff[1] = ~buff[0];
            serial.write(buff, 2);
        } else {
            retrytime = 0;
            bufflen = 0;
            char checksum = 0;
            sscanf(ui->flashsize->currentText().toStdString().c_str(), "%u", &binlen);
            binlen *= 1024;
            unsigned char sectornum = binlen / 4096;
            if (binlen % 4096 != 0) {
                sectornum++;
            }
            chipstep = ISP_WRITEPROTECT_TWO;
            char buff[sectornum+2];
            buff[0] = sectornum - 1;
            checksum ^= buff[0];
            for (i = 0 ; i < sectornum ; i++) {
                buff[i+1] = i;
                checksum ^= buff[i+1];
            }
            buff[sectornum+1] = checksum;
            serial.write(buff, sectornum+2);
        }
    } else if (chipstep == ISP_WRITEPROTECT_TWO) {
        if (bufflen != 1 || (bufflen == 1 && serialReadBuff[0] != 0x79)) {
            retrytime++;
            if (retrytime == 50) {
                ui->tips->appendPlainText("擦除初始化失败");
                CloseSerial();
                btnStatus = BTN_STATUS_IDLE;
                return;
            }
            bufflen = 0;
            chipstep = ISP_WRITEPROTECT;
            char buff[2];
            buff[0] = ISP_WRITEPROTECT;
            buff[1] = ~buff[0];
            serial.write(buff, 2);
        } else {
            retrytime = 0;
            bufflen = 0;
            ui->tips->appendPlainText("写保护完成，请重启单片机与本软件");
            CloseSerial();
            btnStatus = BTN_STATUS_IDLE;
            return;
        }
    } else if (chipstep == ISP_WRITEUNPROTECT || chipstep == ISP_READPROTECT || chipstep == ISP_READUNPROTECT) {
        if (serialReadBuff[0] != 0x79 || (bufflen == 2 && serialReadBuff[1] != 0x79)) {
            retrytime++;
            if (retrytime == 50) {
                ui->tips->appendPlainText("擦除初始化失败");
                CloseSerial();
                btnStatus = BTN_STATUS_IDLE;
                return;
            }
            bufflen = 0;
            char buff[2];
            buff[0] = chipstep;
            buff[1] = ~buff[0];
            serial.write(buff, 2);
        } else if (bufflen == 2 && serialReadBuff[1] == 0x79) {
            retrytime = 0;
            bufflen = 0;
            switch (chipstep) {
                case ISP_WRITEUNPROTECT : ui->tips->appendPlainText("解除写保护完成，请重启单片机与本软件");break;
                case ISP_READPROTECT    : ui->tips->appendPlainText("读保护完成，请重启单片机与本软件");break;
                case ISP_READUNPROTECT  : ui->tips->appendPlainText("解除读保护完成，请重启单片机与本软件");break;
                default:
                    ui->tips->appendPlainText("程序内部错误2");
                    CloseSerial();
                    btnStatus = BTN_STATUS_IDLE;
                    return;
            }
            CloseSerial();
            btnStatus = BTN_STATUS_IDLE;
            return;
        }
    } else if (chipstep == ISP_GETINFO) { // get infomation
        if (serialReadBuff[0] != 0x79 || (bufflen == 5 && serialReadBuff[4] != 0x79)) {
            retrytime++;
            if (retrytime == 50) {
                ui->tips->appendPlainText("擦除初始化失败");
                CloseSerial();
                btnStatus = BTN_STATUS_IDLE;
                return;
            }
            bufflen = 0;
            chipstep = ISP_GETINFO;
            char buff[2];
            buff[0] = ISP_GETINFO;
            buff[1] = ~buff[0];
            serial.write(buff, 2);
        } else if (bufflen == 5) {
            retrytime = 0;
            bufflen = 0;
            char buff[64];
            sprintf(buff, "get version & read protection: 0x%02x 0x%02x 0x%02x", serialReadBuff[1], serialReadBuff[2], serialReadBuff[3]);
            ui->tips->appendPlainText(buff);
            chipstep = ISP_GETINFO_TWO;
            buff[0] = ISP_GETINFO_TWO;
            buff[1] = ~buff[0];
            serial.write(buff, 2);
        }
    } else if (chipstep == ISP_GETINFO_TWO) { // get infomation two
        if (serialReadBuff[0] != 0x79 || (bufflen >= 2 && bufflen == serialReadBuff[1]+4 && serialReadBuff[bufflen-1] != 0x79)) {
            retrytime++;
            if (retrytime == 50) {
                ui->tips->appendPlainText("擦除初始化失败");
                CloseSerial();
                btnStatus = BTN_STATUS_IDLE;
                return;
            }
            bufflen = 0;
            chipstep = ISP_GETINFO_TWO;
            char buff[2];
            buff[0] = ISP_GETINFO_TWO;
            buff[1] = ~buff[0];
            serial.write(buff, 2);
        } else if (bufflen >= 2 && bufflen == serialReadBuff[1]+4) {
            retrytime = 0;
            bufflen = 0;
            char buff[64];
            unsigned char offset = sprintf(buff, "get ID command:");
            for (i = 0 ; i < (unsigned int)serialReadBuff[1]+1 ; i++) {
                unsigned char of = sprintf(buff+offset, " 0x%02x", serialReadBuff[2+i]);
                offset += of;
            }
            ui->tips->appendPlainText(buff);
            CloseSerial();
            btnStatus = BTN_STATUS_IDLE;
            return;
        }
    } else if (chipstep == ISP_ERASE || chipstep == ISP_ERASE_TWO) { // erase
        if (bufflen != 1 || (bufflen == 1 && serialReadBuff[0] != 0x79)) {
            retrytime++;
            if (retrytime == 50) {
                ui->tips->appendPlainText("擦除初始化失败");
                CloseSerial();
                btnStatus = BTN_STATUS_IDLE;
                return;
            }
            bufflen = 0;
            chipstep = ISP_ERASE;
            char buff[2];
            buff[0] = ISP_ERASE;
            buff[1] = ~buff[0];
            serial.write(buff, 2);
        } else {
            retrytime = 0;
            bufflen = 0;
            if (chipstep == ISP_ERASE) {
                ui->tips->appendPlainText("擦除初始化成功，开始擦除");
                chipstep = ISP_ERASE_TWO;
                if (btnStatus == BTN_STATUS_WRITE) {
                    char checksum = 0;
                    unsigned int pagenumbers = binlen / 2048;
                    if (binlen % 2048 != 0) {
                        pagenumbers++;
                    }
                    char buff[pagenumbers+2];
                    buff[0] = pagenumbers - 1;
                    checksum ^= buff[0];
                    for(i = 0 ; i < pagenumbers ; i++) {
                        buff[i+1] = i;
                        checksum ^= buff[i+1];
                    }
                    buff[pagenumbers+1] = checksum;
                    serial.write(buff, pagenumbers+2);
                } else if (btnStatus == BTN_STATUS_ERASE) {
                    char buff[2];
                    buff[0] = 0xff;
                    buff[1] = ~buff[0];
                    serial.write(buff, 2);
                }
            } else if (chipstep == ISP_ERASE_TWO) {
                if (btnStatus == BTN_STATUS_WRITE) {
                    ui->tips->appendPlainText("擦除完毕，开始写入");
                    ui->tips->appendPlainText("已成功写入0k数据");
                    addr = 0;
                    chipstep = ISP_WRITE;
                    char buff[2];
                    buff[0] = ISP_WRITE;
                    buff[1] = ~buff[0];
                    serial.write(buff, 2);
                } else if (btnStatus == BTN_STATUS_ERASE) {
                    ui->tips->appendPlainText("擦除完毕");
                    CloseSerial();
                    btnStatus = BTN_STATUS_IDLE;
                    return;
                }
            } else {
                ui->tips->appendPlainText("程序内部错误3");
                CloseSerial();
                btnStatus = BTN_STATUS_IDLE;
                return;
            }
        }
    } else if (chipstep == ISP_WRITE || chipstep == ISP_WRITE_TWO || chipstep == ISP_WRITE_THREE || chipstep == ISP_WRITE_FOUR) { // write
        if (bufflen != 1 || (bufflen == 1 && serialReadBuff[0] != 0x79)) {
            retrytime++;
            if (retrytime == 50) {
                ui->tips->appendPlainText("写入失败");
                CloseSerial();
                btnStatus = BTN_STATUS_IDLE;
                return;
            }
            bufflen = 0;
            chipstep = ISP_WRITE;
            char buff[2];
            buff[0] = ISP_WRITE;
            buff[1] = ~buff[0];
            serial.write(buff, 2);
        } else {
            retrytime = 0;
            bufflen = 0;
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
                    sprintf(buff, "已成功写入%uk数据", addr / 1024);
                    ui->tips->appendPlainText(buff);
                }
                chipstep = ISP_WRITE;
                buff[0] = ISP_WRITE;
                buff[1] = 0xce;
                serial.write(buff, 2);
            } else if (chipstep == ISP_WRITE_FOUR) {
                if (ui->checkwrite->isChecked()) {
                    char buff[64];
                    sprintf(buff, "全部写入完成，一共写入%uk数据，开始校验", addr / 1024);
                    ui->tips->appendPlainText(buff);
                    ui->tips->appendPlainText("已校验0k数据");
                    addr = 0;
                    chipstep = ISP_READ;
                    buff[0] = ISP_READ;
                    buff[1] = ~buff[0];
                    serial.write(buff, 2);
                } else {
                    char buff[64];
                    sprintf(buff, "全部写入完成，一共写入%uk数据", addr / 1024);
                    ui->tips->appendPlainText(buff);
                    CloseSerial();
                    btnStatus = BTN_STATUS_IDLE;
                    return;
                }
            } else {
                ui->tips->appendPlainText("程序内部错误4");
                CloseSerial();
                btnStatus = BTN_STATUS_IDLE;
                return;
            }
        }
    } else if (chipstep == ISP_READ || chipstep == ISP_READ_TWO) { // read
        if (bufflen != 1 || (bufflen == 1 && serialReadBuff[0] != 0x79)) {
            retrytime++;
            if (retrytime == 50) {
                ui->tips->appendPlainText("读取失败");
                CloseSerial();
                btnStatus = BTN_STATUS_IDLE;
                return;
            }
            bufflen = 0;
            chipstep = ISP_READ;
            char buff[2];
            buff[0] = ISP_READ;
            buff[1] = ~buff[0];
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
    } else if (chipstep == ISP_READ_THREE) { // read
        if (serialReadBuff[0] != 0x79) {
            retrytime++;
            if (retrytime == 50) {
                ui->tips->appendPlainText("读取失败");
                CloseSerial();
                btnStatus = BTN_STATUS_IDLE;
                return;
            }
            bufflen = 0;
            chipstep = ISP_READ;
            char buff[2];
            buff[0] = ISP_READ;
            buff[1] = ~buff[0];
            serial.write(buff, 2);
        } else if (bufflen == 257) {
            retrytime = 0;
            bufflen = 0;
            char buff[64];
            if (btnStatus == BTN_STATUS_WRITE) {
                for (i = 0 ; i < 256 ; i++) {
                    if (bin[addr+i] != serialReadBuff[i+1]) {
                        sprintf(buff, "校验失败, addr:%u,i:%u,bin:0x%02x,buff:0x%02x", addr, i, bin[addr+i], serialReadBuff[i+1]);
                        ui->tips->appendPlainText(buff);
                        CloseSerial();
                        btnStatus = BTN_STATUS_IDLE;
                        return;
                    }
                }
            } else if (btnStatus == BTN_STATUS_READ) {
                memcpy(bin+addr, serialReadBuff+1, 256);
            } else {
                ui->tips->appendPlainText("程序内部错误5");
                CloseSerial();
                btnStatus = BTN_STATUS_IDLE;
                return;
            }
            addr += 256;
            if (addr % 2048 == 0) {
                QTextCursor pos = ui->tips->textCursor();
                pos.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
                pos.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
                pos.removeSelectedText();
                pos.deletePreviousChar();
                if (btnStatus == BTN_STATUS_WRITE) {
                    sprintf(buff, "已校验%uk数据", addr / 1024);
                } else if (btnStatus == BTN_STATUS_READ) {
                    sprintf(buff, "已读出%uk数据", addr / 1024);
                } else {
                    ui->tips->appendPlainText("程序内部错误6");
                    CloseSerial();
                    btnStatus = BTN_STATUS_IDLE;
                    return;
                }
                ui->tips->appendPlainText(buff);
            }
            chipstep = ISP_READ;
            buff[0] = ISP_READ;
            buff[1] = 0xee;
            serial.write(buff, 2);
        }
    } else if (chipstep == ISP_READ_FOUR) { // read
        if (serialReadBuff[0] != 0x79) {
            retrytime++;
            if (retrytime == 50) {
                ui->tips->appendPlainText("读取失败");
                CloseSerial();
                btnStatus = BTN_STATUS_IDLE;
                return;
            }
            bufflen = 0;
            chipstep = ISP_READ;
            char buff[2];
            buff[0] = ISP_READ;
            buff[1] = ~buff[0];
            serial.write(buff, 2);
        } else if (bufflen == binlen-addr+1) {
            retrytime = 0;
            bufflen = 0;
            char buff[64];
            if (btnStatus == BTN_STATUS_WRITE) {
                for (i = 0 ; i < binlen-addr ; i++) {
                    if (bin[addr+i] != serialReadBuff[i+1]) {
                        sprintf(buff, "校验失败, addr:%u,i:%u,bin:0x%02x,buff:0x%02x", addr, i, bin[addr+i], serialReadBuff[i+1]);
                        ui->tips->appendPlainText(buff);
                        CloseSerial();
                        btnStatus = BTN_STATUS_IDLE;
                        return;
                    }
                }
                addr = binlen;
                sprintf(buff, "全部校验完成，一共校验%uk数据", addr / 1024);
            } else if (btnStatus == BTN_STATUS_READ) {
                memcpy(bin+addr, serialReadBuff+1, binlen-addr);
                addr = binlen;
                sprintf(buff, "全部读出完成，一共读出%uk数据", addr / 1024);
                // 这里将文件写入
                QFile file(savefilepath);
                if (!file.open(QIODevice::WriteOnly)) {
                    sprintf(buff, "文件打开失败");
                } else {
                    if (file.write((char*)bin, addr) < addr) {
                        sprintf(buff, "文件写入不完整");
                    } else {
                        sprintf(buff, "镜像保存成功");
                    }
                    file.close();
                }
            } else {
                sprintf(buff, "程序内部错误7");
            }
            ui->tips->appendPlainText(buff);
            CloseSerial();
            btnStatus = BTN_STATUS_IDLE;
            return;
        }
    } else if (chipstep != ISP_READ_THREE && chipstep != ISP_READ_FOUR){
        char buff[64];
        sprintf(buff, "chipstep:0x%02x, bufflen:%d, ack:0x%02x", chipstep, bufflen, serialReadBuff[0]);
        ui->tips->appendPlainText(buff);
        CloseSerial();
        btnStatus = BTN_STATUS_IDLE;
        return;
    }
    timer.stop();
    timer.start(5000);
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
    timer.start(5000);
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
    if (!filepath.length()) {
        ui->tips->appendPlainText("保存文件路径配置错误");
        return;
    }
    savefilepath = filepath;
    sscanf(ui->flashsize->currentText().toStdString().c_str(), "%u", &binlen);
    binlen *= 1024;
    btnStatus = BTN_STATUS_READ;
    retrytime = 0;
    if (issynced) {
        addr = 0;
        chipstep = ISP_READ;
        char buff[2];
        buff[0] = ISP_READ;
        buff[1] = ~buff[0];
        if (OpenSerial(buff, 2)) {
            ui->tips->appendPlainText("串口打开失败");
            return;
        }
        ui->tips->appendPlainText("开始读取操作");
    } else {
        chipstep = ISP_SYNC; // sync
        char buff[1];
        buff[0] = ISP_SYNC;
        if (OpenSerial(buff, 1)) {
            ui->tips->appendPlainText("串口打开失败");
            return;
        }
        ui->tips->appendPlainText("开始同步串口");
    }
}

void Stm32Isp::on_readchipmsg_clicked() {
    btnStatus = BTN_STATUS_READ_MSG;
    retrytime = 0;
    if (issynced) {
        chipstep = ISP_GETINFO;
        char buff[2];
        buff[0] = ISP_GETINFO;
        buff[1] = ~buff[0];
        if (OpenSerial(buff, 2)) {
            ui->tips->appendPlainText("串口打开失败");
            return;
        }
        ui->tips->appendPlainText("开始擦除操作");
    } else {
        chipstep = ISP_SYNC; // sync
        char buff[1];
        buff[0] = ISP_SYNC;
        if (OpenSerial(buff, 1)) {
            ui->tips->appendPlainText("串口打开失败");
            return;
        }
        ui->tips->appendPlainText("开始同步串口");
    }
}

void Stm32Isp::on_writechip_clicked() {
    QString filepath = ui->filepath->text();
    int pos = filepath.lastIndexOf(".");
    QString suffix = filepath.mid(pos);
    QFile file(filepath);
    if(!file.open(QIODevice::ReadOnly)) {
        ui->tips->appendPlainText("无法读取烧录文件");
        return;
    }
    QByteArray bytedata = file.readAll();
    file.close();
    unsigned char *chardata = (unsigned char*)bytedata.data();
    if (!suffix.compare(".hex")) {
        if (HexToBin(chardata, bytedata.length(), bin, sizeof(bin), 0x08000000, &binlen) != RES_OK) {
            ui->tips->appendPlainText("hex文件格式错误");
            return;
        }
    } else if (!suffix.compare(".bin")) {
        uint len = bytedata.length();
        if (len > sizeof(bin)) {
            ui->tips->appendPlainText("bin文件太大");
            return;
        }
        memcpy(bin, chardata, len);
        binlen = len;
    } else {
        ui->tips->appendPlainText("文件类型错误");
        return;
    }
/*
    while (binlen % 4 != 0) {
        bin[binlen] = 0xff;
        binlen++;
    }
*/
    btnStatus = BTN_STATUS_WRITE;
    retrytime = 0;
    if (issynced) {
        chipstep = ISP_ERASE;
        char buff[2];
        buff[0] = ISP_ERASE;
        buff[1] = ~buff[0];
        if (OpenSerial(buff, 2)) {
            ui->tips->appendPlainText("串口打开失败");
            return;
        }
        ui->tips->appendPlainText("开始擦除操作");
    } else {
        chipstep = ISP_SYNC; // sync
        char buff[1];
        buff[0] = ISP_SYNC;
        if (OpenSerial(buff, 1)) {
            ui->tips->appendPlainText("串口打开失败");
            return;
        }
        ui->tips->appendPlainText("开始同步串口");
    }
}

void Stm32Isp::on_erasechip_clicked() {
    btnStatus = BTN_STATUS_ERASE;
    retrytime = 0;
    if (issynced) {
        chipstep = ISP_ERASE;
        char buff[2];
        buff[0] = ISP_ERASE;
        buff[1] = ~buff[0];
        if (OpenSerial(buff, 2)) {
            ui->tips->appendPlainText("串口打开失败");
            return;
        }
        ui->tips->appendPlainText("开始擦除操作");
    } else {
        chipstep = ISP_SYNC; // sync
        char buff[1];
        buff[0] = ISP_SYNC;
        if (OpenSerial(buff, 1)) {
            ui->tips->appendPlainText("串口打开失败");
            return;
        }
        ui->tips->appendPlainText("开始同步串口");
    }
}

void Stm32Isp::on_clearlog_clicked() {
    ui->tips->clear();
}

void Stm32Isp::on_writeprotect_clicked() {
    btnStatus = BTN_STATUS_WRITE_PROTECT;
    retrytime = 0;
    if (issynced) {
        chipstep = ISP_WRITEPROTECT;
        char buff[2];
        buff[0] = ISP_WRITEPROTECT;
        buff[1] = ~buff[0];
        if (OpenSerial(buff, 2)) {
            ui->tips->appendPlainText("串口打开失败");
            return;
        }
        ui->tips->appendPlainText("开始添加写保护操作");
    } else {
        chipstep = ISP_SYNC; // sync
        char buff[1];
        buff[0] = ISP_SYNC;
        if (OpenSerial(buff, 1)) {
            ui->tips->appendPlainText("串口打开失败");
            return;
        }
        ui->tips->appendPlainText("开始同步串口");
    }
}

void Stm32Isp::on_writeunprotect_clicked() {
    btnStatus = BTN_STATUS_WRITE_UNPROTECT;
    retrytime = 0;
    if (issynced) {
        chipstep = ISP_WRITEUNPROTECT;
        char buff[2];
        buff[0] = ISP_WRITEUNPROTECT;
        buff[1] = ~buff[0];
        if (OpenSerial(buff, 2)) {
            ui->tips->appendPlainText("串口打开失败");
            return;
        }
        ui->tips->appendPlainText("开始解除写保护操作");
    } else {
        chipstep = ISP_SYNC; // sync
        char buff[1];
        buff[0] = ISP_SYNC;
        if (OpenSerial(buff, 1)) {
            ui->tips->appendPlainText("串口打开失败");
            return;
        }
        ui->tips->appendPlainText("开始同步串口");
    }
}

void Stm32Isp::on_readprotect_clicked() {
    btnStatus = BTN_STATUS_READ_PROTECT;
    retrytime = 0;
    if (issynced) {
        chipstep = ISP_READPROTECT;
        char buff[2];
        buff[0] = ISP_READPROTECT;
        buff[1] = ~buff[0];
        if (OpenSerial(buff, 2)) {
            ui->tips->appendPlainText("串口打开失败");
            return;
        }
        ui->tips->appendPlainText("开始添加读操作");
    } else {
        chipstep = ISP_SYNC; // sync
        char buff[1];
        buff[0] = ISP_SYNC;
        if (OpenSerial(buff, 1)) {
            ui->tips->appendPlainText("串口打开失败");
            return;
        }
        ui->tips->appendPlainText("开始同步串口");
    }
}

void Stm32Isp::on_readunprotect_clicked() {
    btnStatus = BTN_STATUS_READ_UNPROTECT;
    retrytime = 0;
    if (issynced) {
        chipstep = ISP_READUNPROTECT;
        char buff[2];
        buff[0] = ISP_READUNPROTECT;
        buff[1] = ~buff[0];
        if (OpenSerial(buff, 2)) {
            ui->tips->appendPlainText("串口打开失败");
            return;
        }
        ui->tips->appendPlainText("开始解除读保护操作");
    } else {
        chipstep = ISP_SYNC; // sync
        char buff[1];
        buff[0] = ISP_SYNC;
        if (OpenSerial(buff, 1)) {
            ui->tips->appendPlainText("串口打开失败");
            return;
        }
        ui->tips->appendPlainText("开始同步串口");
    }
}
