#include "uartassist.h"
#include "ui_uartassist.h"
#include <QDateTime>

UartAssist::UartAssist(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UartAssist) {
    ui->setupUi(this);
    GetComList();
}

UartAssist::~UartAssist() {    
    delete ui;
}

// 获取可用的串口号
void UartAssist::GetComList () {
    QComboBox *c = ui->com;
    for (int a = c->count() ; a > 0 ; a--) {
        c->removeItem(a-1);
    }
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        c->addItem(info.portName() + " " + info.description());
    }
}

void UartAssist::on_refresh_clicked() {
    if (ui->startclose->text() == "停止") {
        return;
    }
    GetComList();
}

int UartAssist::GetBtnStatus () {
    return btnStatus;
}

void UartAssist::ReadSerialData() {
    QByteArray arr = serial.readAll();
    if (ui->islogmode->isChecked()) {
        QDateTime current_date_time = QDateTime::currentDateTime();
        QString txt = ui->sendEdit->toPlainText();
        ui->receiveEdit->setAlignment(Qt::AlignLeft);
        ui->receiveEdit->setTextColor(Qt::blue);
        ui->receiveEdit->append(current_date_time.toString("[yyyy-MM-dd hh:mm:ss.zzz]"));
        ui->receiveEdit->setTextColor(Qt::black);
        ui->receiveEdit->append(arr);
        ui->receiveEdit->append("");
    } else if (ui->autonewline->isChecked()) {
        ui->receiveEdit->append(arr);
    } else {
        ui->receiveEdit->moveCursor(QTextCursor::End);
        ui->receiveEdit->insertPlainText(arr);
    }
}

void UartAssist::on_startclose_clicked() {
    if (ui->startclose->text() == "开始") {
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
            ui->receiveEdit->setAlignment(Qt::AlignLeft);
            ui->receiveEdit->setTextColor(Qt::black);
            ui->receiveEdit->append("串口打开失败");
            ui->receiveEdit->append("");
            return;
        }
        connect(&serial, SIGNAL(readyRead()), this, SLOT(ReadSerialData()));
        ui->startclose->setText("停止");
        btnStatus = 1;
        ui->com->setEnabled(false);
        ui->baudrate->setEnabled(false);
        ui->parity->setEnabled(false);
        ui->databit->setEnabled(false);
        ui->stopbit->setEnabled(false);
        ui->flowcontrol->setEnabled(false);
    } else if (ui->startclose->text() == "停止") {
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

void UartAssist::on_send_clicked() {
    if (ui->startclose->text() == "开始") {
        ui->receiveEdit->setAlignment(Qt::AlignLeft);
        ui->receiveEdit->setTextColor(Qt::black);
        ui->receiveEdit->append("串口还没打开，请先打开串口");
        ui->receiveEdit->append("");
        return;
    }
    QString txt = ui->sendEdit->toPlainText();
    if (ui->atnewline->isChecked()) {
        int len = txt.length();
        char dat[len+2];
        memcpy(dat, txt.toStdString().c_str(), len);
        dat[len++] = '\r';
        dat[len++] = '\n';
        serial.write(dat, len);
    } else {
        serial.write(txt.toStdString().c_str(), txt.length());
    }
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
}

void UartAssist::on_clearsend_clicked() {
    ui->sendEdit->clear();
}

void UartAssist::on_clearreceive_clicked() {
    ui->receiveEdit->clear();
}
