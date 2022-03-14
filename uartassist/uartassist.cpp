#include "uartassist.h"
#include "ui_uartassist.h"
#include <QDateTime>
#include <QTextCodec>
// 公共函数库
#include "common/hextobin.h"

UartAssist::UartAssist (QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UartAssist) {
    ui->setupUi(this);
    GetComList();
}

UartAssist::~UartAssist () {
    delete ui;
}

// 获取可用的串口号
void UartAssist::GetComList () {
    QComboBox *c = ui->com;
    c->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        c->addItem(info.portName() + " " + info.description());
    }
}

void UartAssist::on_refresh_clicked () {
    if (ui->startclose->text() == "停止") {
        return;
    }
    GetComList();
}

int UartAssist::GetBtnStatus () {
    return btnStatus;
}

void UartAssist::ReadSerialData () {
    QByteArray ba = serial.readAll();
    if (ui->receivenoshow->isChecked()) {
        return;
    }
    if (ui->receiveHex->isChecked()) {
        uchar *s = (uchar*)ba.data();
        int size = ba.size();
        char dat[3*size];
        for (int i = 0 ; i < size ; i++) {
            uchar tmp = s[i] >> 4;
            dat[3*i] = tmp < 10 ? tmp + '0' : tmp + 'A' - 10;
            tmp = s[i] & 0x0f;
            dat[3*i+1] = tmp < 10 ? tmp + '0' : tmp + 'A' - 10;
            dat[3*i+2] = ' ';
        }
        ba = QByteArray(dat, 3*size);
    }
    QTextCodec *tc = QTextCodec::codecForName("UTF8");
    QString str = tc->toUnicode(ba);
    if (ui->islogmode->isChecked()) {
        QDateTime current_date_time = QDateTime::currentDateTime();
        ui->receiveEdit->setTextColor(Qt::blue);
        ui->receiveEdit->append(current_date_time.toString("[yyyy-MM-dd hh:mm:ss.zzz]"));
        ui->receiveEdit->setTextColor(Qt::black);
        ui->receiveEdit->append(str);
        ui->receiveEdit->append("");
    } else if (ui->autonewline->isChecked()) {
        ui->receiveEdit->append(str);
    } else {
        ui->receiveEdit->moveCursor(QTextCursor::End);
        ui->receiveEdit->insertPlainText(str);
    }
}

void UartAssist::on_startclose_clicked () {
    if (btnStatus == 0) {
        char comname[12];
        QSerialPort::BaudRate baudrate;
        QSerialPort::Parity parity;
        QSerialPort::DataBits databit;
        QSerialPort::StopBits stopbit;
        QSerialPort::FlowControl flowcontrol;
        unsigned int tmp;
        sscanf(ui->com->currentText().toStdString().c_str(), "%s ", comname);
        sscanf(ui->baudrate->currentText().toStdString().c_str(), "%u", &tmp);
        switch (tmp) {
            case  1200:baudrate = QSerialPort::Baud1200;break;
            case  2400:baudrate = QSerialPort::Baud2400;break;
            case  4800:baudrate = QSerialPort::Baud4800;break;
            case  9600:baudrate = QSerialPort::Baud9600;break;
            case 19200:baudrate = QSerialPort::Baud19200;break;
            case 38400:baudrate = QSerialPort::Baud38400;break;
            case 57600:baudrate = QSerialPort::Baud57600;break;
            default   :baudrate = QSerialPort::Baud115200;break;
        }
        if (ui->parity->currentText() == "ODD") {
            parity = QSerialPort::OddParity;
        } else if (ui->parity->currentText() == "EVEN") {
            parity = QSerialPort::EvenParity;
        } else if (ui->parity->currentText() == "MARK") {
            parity = QSerialPort::MarkParity;
        } else if (ui->parity->currentText() == "SPACE") {
            parity = QSerialPort::SpaceParity;
        } else {
            parity = QSerialPort::NoParity;
        }
        sscanf(ui->databit->currentText().toStdString().c_str(), "%u", &tmp);
        switch (tmp) {
            case  5:databit = QSerialPort::Data5;break;
            case  6:databit = QSerialPort::Data6;break;
            case  7:databit = QSerialPort::Data7;break;
            default:databit = QSerialPort::Data8;break;
        }
        if (ui->stopbit->currentText() == "1.5") {
            stopbit = QSerialPort::OneAndHalfStop;
        } else if (ui->stopbit->currentText() == "2") {
            stopbit = QSerialPort::TwoStop;
        } else {
            stopbit = QSerialPort::OneStop;
        }
        if (ui->flowcontrol->currentText() == "RTS/CTS") {
            flowcontrol = QSerialPort::HardwareControl;
        } else if (ui->flowcontrol->currentText() == "XON/XOFF") {
            flowcontrol = QSerialPort::SoftwareControl;
        } else {
            flowcontrol = QSerialPort::NoFlowControl;
        }
        serial.setPortName(comname);
        serial.setBaudRate(baudrate);
        serial.setParity(parity);
        serial.setDataBits(databit);
        serial.setStopBits(stopbit);
        serial.setFlowControl(flowcontrol);
        if (!serial.open(QIODevice::ReadWrite)) {
            ui->receiveEdit->append("串口打开失败");
            ui->receiveEdit->append("");
            return;
        }
        serial.clear();
        connect(&serial, SIGNAL(readyRead()), this, SLOT(ReadSerialData()));
        ui->startclose->setText("停止");
        btnStatus = 1;
        ui->com->setEnabled(false);
        ui->baudrate->setEnabled(false);
        ui->parity->setEnabled(false);
        ui->databit->setEnabled(false);
        ui->stopbit->setEnabled(false);
        ui->flowcontrol->setEnabled(false);
    } else if (btnStatus == 1) {
        disconnect(&serial, 0, 0, 0);
        serial.close();
        ui->startclose->setText("开始");
        btnStatus = 0;
        ui->com->setEnabled(true);
        ui->baudrate->setEnabled(true);
        ui->parity->setEnabled(true);
        ui->databit->setEnabled(true);
        ui->stopbit->setEnabled(true);
        ui->flowcontrol->setEnabled(true);
    }
}

void UartAssist::on_send_clicked () {
    if (btnStatus == 0) {
        ui->receiveEdit->append("串口未打开，请先打开串口");
        ui->receiveEdit->append("");
        return;
    }
    QString txt = ui->sendEdit->toPlainText();
    QByteArray ba = txt.toUtf8();
    char *s = ba.data();
    int len = ba.size();
    char dat[len+2];
    if (ui->sendHex->isChecked()) {
        int n = 0;
        for (int i = 1 ; i < len ; i+=3) {
            if (i > 2 && s[i-2] != ' ') {
                ui->receiveEdit->append("发送数据格式错误");
                return;
            }
            uchar tmp1 = HexCharToBinChar(s[i-1]);
            if (tmp1 == 0xff) {
                ui->receiveEdit->append("发送数据格式错误");
                return;
            }
            uchar tmp2 = HexCharToBinChar(s[i]);
            if (tmp2 == 0xff) {
                ui->receiveEdit->append("发送数据格式错误");
                return;
            }
            dat[n++] = (tmp1<<4) + tmp2;
        }
        len = n;
    } else {
        memcpy(dat, s, len);
    }
    if (ui->atnewline->isChecked()) {
        dat[len++] = '\r';
        dat[len++] = '\n';
    }
    serial.write(dat, len);
    if (!ui->islogmode->isChecked()) {
        return;
    }
    QDateTime current_date_time = QDateTime::currentDateTime();
    ui->receiveEdit->setAlignment(Qt::AlignRight);
    ui->receiveEdit->setTextColor(Qt::green);
    ui->receiveEdit->append(current_date_time.toString("[yyyy-MM-dd hh:mm:ss.zzz]"));
    ui->receiveEdit->setTextColor(Qt::black);
    ui->receiveEdit->append(txt);
    ui->receiveEdit->append("");
    ui->receiveEdit->setAlignment(Qt::AlignLeft);
}

void UartAssist::on_clearsend_clicked () {
    ui->sendEdit->clear();
}

void UartAssist::on_clearreceive_clicked () {
    ui->receiveEdit->clear();
}

void UartAssist::on_sendHex_clicked () {
    QString txt = ui->sendEdit->toPlainText();
    QByteArray ba = txt.toUtf8();
    if (ui->sendHex->isChecked()) {
        uchar *s = (uchar*)ba.data();
        int size = ba.size();
        char dat[3*size];
        for (int i = 0 ; i < size ; i++) {
            uchar tmp = s[i] >> 4;
            dat[3*i] = tmp < 10 ? tmp + '0' : tmp + 'A' - 10;
            tmp = s[i] & 0x0f;
            dat[3*i+1] = tmp < 10 ? tmp + '0' : tmp + 'A' - 10;
            dat[3*i+2] = ' ';
        }
        ba = QByteArray(dat, 3*size);
    } else {
        char *s = ba.data();
        int len = ba.size();
        char dat[len/3+1];
        int n = 0;
        for (int i = 1 ; i < len ; i+=3) {
            if (i > 2 && s[i-2] != ' ') {
                return;
            }
            uchar tmp1 = HexCharToBinChar(s[i-1]);
            if (tmp1 == 0xff) {
                return;
            }
            uchar tmp2 = HexCharToBinChar(s[i]);
            if (tmp2 == 0xff) {
                return;
            }
            dat[n++] = (tmp1<<4) + tmp2;
        }
        ba = QByteArray(dat, n);
    }
    QTextCodec *tc = QTextCodec::codecForName("UTF8");
    QString str = tc->toUnicode(ba);
    ui->sendEdit->clear();
    ui->sendEdit->appendPlainText(str);
}
