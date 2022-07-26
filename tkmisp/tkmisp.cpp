#include "tkmisp.h"
#include "ui_tkmisp.h"
#include <QFileDialog>

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
    if (chipstep == ISP_SYNC) { // sync
        retrytime++;
        if (retrytime < 250) {
            char buff[64];
            buff[0] = ISP_SYNC;
            serial.write(buff, 1);
            QTextCursor pos = ui->tips->textCursor();
            pos.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
            pos.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            pos.removeSelectedText();
            pos.deletePreviousChar();
            sprintf(buff, "开始同步串口...%u", retrytime);
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
}

int TkmIsp::OpenSerial () {
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
    }
}

void TkmIsp::on_writechip_clicked () {
    if (btnStatus) {
        ui->tips->appendPlainText("用户终止操作");
        CloseSerial();
        return;
    }
    bool notfoundload = true;
    QByteArray msu0bytedata, msu1bytedata;
    QString msu0suffix, msu1suffix;
    if (ui->msu0load->isChecked()) {
        QString filepath = ui->msu0filepath->text();
        QFile file(filepath);
        if(!file.open(QIODevice::ReadOnly)) {
            ui->tips->appendPlainText("无法读取烧录文件");
            return;
        }
        msu0bytedata = file.readAll();
        file.close();
        int pos = filepath.lastIndexOf(".");
        msu0suffix = filepath.mid(pos);
        notfoundload = false;
    }
    if (ui->msu1load->isChecked()) {
        QString filepath = ui->msu1filepath->text();
        QFile file(filepath);
        if(!file.open(QIODevice::ReadOnly)) {
            ui->tips->appendPlainText("无法读取烧录文件");
            return;
        }
        msu1bytedata = file.readAll();
        file.close();
        int pos = filepath.lastIndexOf(".");
        msu1suffix = filepath.mid(pos);
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
    sprintf(buff, "开始同步串口...%u", ++retrytime);
    ui->tips->appendPlainText(buff);
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
    btnStatus = BTN_STATUS_READ;
    sprintf(buff, "开始同步串口...%u", ++retrytime);
    ui->tips->appendPlainText(buff);
}

void TkmIsp::on_erasechip_clicked () {
}

void TkmIsp::on_clearlog_clicked () {
}

void TkmIsp::on_msu1readchip_clicked() {
}


void TkmIsp::on_readchipmsg_clicked() {
}

