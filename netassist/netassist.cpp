#include "netassist.h"
#include "ui_netassist.h"
#include <QDateTime>
#include <QTextCodec>
// 公共函数库
#include "common/hextobin.h"
#include "common/common.h"

NetAssist::NetAssist (QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NetAssist) {
    ui->setupUi(this);
}

NetAssist::~NetAssist () {
    delete ui;
}

int NetAssist::GetBtnStatus () {
    return btnStatus;
}

void NetAssist::TimerOutEvent () {
    timer.stop();
    newdata = 1;
    ui->receiveEdit->append("");
}

void NetAssist::ReadSocketData () {
    timer.stop();
    QByteArray ba;
    QHostAddress srcAddress;
    quint16 srcPort;
    QString prot = ui->protocol->currentText();
    if (prot == "TCP Client") {
        ba = tcpClient.readAll();
        srcAddress = tcpClient.peerAddress();
        srcPort = tcpClient.peerPort();
    } else if (prot == "TCP Server") {
        QTcpSocket *tc = (QTcpSocket*)sender();
        ba = tc->readAll();
        srcAddress = tc->peerAddress();
        srcPort = tc->peerPort();
    } else { // prot == "UDP"
        int size = 0;
        while(udpSocket.hasPendingDatagrams()) {
            int lastsize = size;
            int datasize = udpSocket.pendingDatagramSize();
            size += datasize;
            ba.resize(size);
            udpSocket.readDatagram(ba.data() + lastsize, datasize, &srcAddress, &srcPort);
        }
    }
    if (ui->receivenoshow->isChecked()) {
        return;
    }
    if (ui->receiveHex->isChecked()) {
        uchar *s = (uchar*)ba.data();
        int i, size = ba.size();
        char dat[3*size];
        for (i = 0 ; i < size ; i++) {
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
        if (newdata) {
            newdata = 0;
            char srcPortStr[8];
            sprintf(srcPortStr, "%u", srcPort);
            QDateTime current_date_time = QDateTime::currentDateTime();
            ui->receiveEdit->setTextColor(Qt::blue);
            ui->receiveEdit->append(current_date_time.toString("[yyyy-MM-dd hh:mm:ss.zzz]") + "from " + srcAddress.toString() + ":" + srcPortStr);
            ui->receiveEdit->setTextColor(Qt::black);
            ui->receiveEdit->append(str);
        } else {
            ui->receiveEdit->moveCursor(QTextCursor::End);
            ui->receiveEdit->insertPlainText(str);
        }
    } else if (ui->autonewline->isChecked()) {
        ui->receiveEdit->append(str);
    } else {
        ui->receiveEdit->moveCursor(QTextCursor::End);
        ui->receiveEdit->insertPlainText(str);
    }
    timer.start(20);
}

void NetAssist::DisconnectSocket () {
    QTcpSocket *tc = (QTcpSocket*)sender();
    OpenCloseSocket(0, tc);
}

void NetAssist::AcceptNewConnect () {
    QTcpSocket *tc = tcpServer.nextPendingConnection();
    connect(tc, SIGNAL(readyRead()), this, SLOT(ReadSocketData()));
    connect(tc, SIGNAL(disconnected()), this, SLOT(DisconnectSocket()));
    clients.append(tc);
    QHostAddress srcAddress = tc->peerAddress();
    quint16 srcPort = tc->peerPort();
    char srcPortStr[8];
    sprintf(srcPortStr, "%u", srcPort);
    ui->clientlist->addItem(srcAddress.toString() + ":" + srcPortStr);
}

void NetAssist::OpenCloseSocket (int state, QTcpSocket *tc) {
    if (state == 1) {
        QString prot = ui->protocol->currentText();
        if (prot == "TCP Client") {
            QString remoteaddr = ui->remoteaddr->text();
            if (!remoteaddr.size()) {
                ui->receiveEdit->append("远程地址为空");
                ui->receiveEdit->append("");
                return;
            }
            tcpClient.connectToHost(remoteaddr, ui->remoteport->value());
            if (!tcpClient.waitForConnected(3000)) {
                ui->receiveEdit->append("连接失败");
                ui->receiveEdit->append("");
                return;
            }
            ui->protocol->setEnabled(false);
            ui->remoteaddr->setEnabled(false);
            ui->remoteport->setEnabled(false);
            ui->startclose->setText("停止");
            btnStatus = 1;
            connect(&timer, SIGNAL(timeout()), this, SLOT(TimerOutEvent()));
            connect(&tcpClient, SIGNAL(readyRead()), this, SLOT(ReadSocketData()));
            connect(&tcpClient, SIGNAL(disconnected()),this,SLOT(DisconnectSocket()));
        } else if (prot == "TCP Server") {
            if (!tcpServer.listen(QHostAddress::Any, ui->localport->value())) {
                ui->receiveEdit->append("启动失败");
                ui->receiveEdit->append("");
                return;
            }
            ui->protocol->setEnabled(false);
            ui->localport->setEnabled(false);
            ui->startclose->setText("停止");
            ui->clientlist->setEnabled(true);
            btnStatus = 1;
            connect(&timer, SIGNAL(timeout()), this, SLOT(TimerOutEvent()));
            connect(&tcpServer, SIGNAL(newConnection()), this, SLOT(AcceptNewConnect()));
        } else { // prot == "UDP"
            if (!udpSocket.open(QIODevice::ReadWrite)) {
                ui->receiveEdit->append("打开套接字失败");
                ui->receiveEdit->append("");
                return;
            }
            if (!udpSocket.bind(QHostAddress::Any, ui->localport->value())) {
                udpSocket.close();
                ui->receiveEdit->append("绑定失败");
                ui->receiveEdit->append("");
                return;
            }
            ui->protocol->setEnabled(false);
            ui->localport->setEnabled(false);
            ui->startclose->setText("停止");
            btnStatus = 1;
            connect(&timer, SIGNAL(timeout()), this, SLOT(TimerOutEvent()));
            connect(&udpSocket, SIGNAL(readyRead()), this, SLOT(ReadSocketData()));
        }
    } else {
        QString prot = ui->protocol->currentText();
        if (prot == "TCP Client") {
            disconnect(&timer, 0, 0, 0);
            timer.stop();
            disconnect(&tcpClient, 0, 0, 0);
            tcpClient.disconnectFromHost();
            if (tcpClient.state() != QTcpSocket::UnconnectedState) {
                tcpClient.waitForDisconnected(3000);
            }
            btnStatus = 0;
            ui->remoteaddr->setEnabled(true);
            ui->remoteport->setEnabled(true);
            ui->startclose->setText("开始");
        } else if (prot == "TCP Server") {
            if (tc != nullptr) {
                disconnect(tc, 0, 0, 0);
                tc->close();
                int row = clients.indexOf(tc);
                clients.remove(row);
                ui->clientlist->removeItem(row);
            } else {
                int i, size = clients.size();
                for (i = 0; i < size; i++) {
                    tc = clients.at(i);
                    disconnect(tc, 0, 0, 0);
                    tc->close();
                }
                disconnect(&timer, 0, 0, 0);
                timer.stop();
                clients.clear();
                disconnect(&tcpServer, 0, 0, 0);
                tcpServer.close();
                btnStatus = 0;
                ui->startclose->setText("开始");
                ui->localport->setEnabled(true);
                ui->clientlist->clear();
                ui->clientlist->setEnabled(false);
            }
        } else { // prot == "UDP"
            disconnect(&timer, 0, 0, 0);
            timer.stop();
            disconnect(&udpSocket, 0, 0, 0);
            udpSocket.close();
            btnStatus = 0;
            ui->startclose->setText("开始");
            ui->localport->setEnabled(true);
        }
        ui->protocol->setEnabled(true);
    }
}

void NetAssist::on_protocol_activated (const QString &prot) {
    if (prot == "TCP Client") {
        ui->remoteaddr->setEnabled(true);
        ui->remoteport->setEnabled(true);
        ui->localport->setEnabled(false);
    } else if (prot == "TCP Server") {
        ui->remoteaddr->setEnabled(false);
        ui->remoteport->setEnabled(false);
        ui->localport->setEnabled(true);
    } else { // prot == "UDP"
        ui->remoteaddr->setEnabled(true);
        ui->remoteport->setEnabled(true);
        ui->localport->setEnabled(true);
    }
}

void NetAssist::on_startclose_clicked () {
    if (btnStatus == 0) {
        OpenCloseSocket(1, nullptr);
    } else if (btnStatus == 1) {
        OpenCloseSocket(0, nullptr);
    }
}

void NetAssist::on_send_clicked () {
    if (!btnStatus) {
        ui->receiveEdit->append("网络尚未连接，发送失败");
        ui->receiveEdit->append("");
        return;
    }
    QString txt = ui->sendEdit->toPlainText();
    QByteArray ba = txt.toUtf8();
    char *s = ba.data();
    int len = ba.size();
    char dat[len+4];
    if (ui->sendHex->isChecked()) {
        int i, n = 0;
        for (i = 1 ; i < len ; i+=3) {
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
    if (ui->sendcrc->isChecked()) {
        ushort crc = 0xffff;
        crc = COMMON::crc_calc(crc, (uchar*)dat, len);
        dat[len++] = crc & 0xff;
        dat[len++] = crc >> 8;
    }
    QHostAddress dstAddress;
    quint16 dstPort;
    QString prot = ui->protocol->currentText();
    if (prot == "TCP Client") {
        tcpClient.write(dat, len);
        dstAddress = tcpClient.peerAddress();
        dstPort = tcpClient.peerPort();
    } else if (prot == "TCP Server") {
        int row = ui->clientlist->currentIndex();
        if (row < 0) {
            ui->receiveEdit->append("没有客户端连接");
            ui->receiveEdit->append("");
            return;
        }
        QTcpSocket *tc = clients.at(row);
        tc->write(dat, len);
        dstAddress = tc->peerAddress();
        dstPort = tc->peerPort();
    } else { // prot == "UDP"
        QString remoteaddr = ui->remoteaddr->text();
        if (!remoteaddr.size()) {
            ui->receiveEdit->append("远程地址为空");
            ui->receiveEdit->append("");
            return;
        }
        dstAddress = QHostAddress(remoteaddr);
        dstPort = ui->remoteport->value();
        udpSocket.writeDatagram(dat, len, dstAddress, dstPort);
    }
    if (!ui->islogmode->isChecked()) {
        return;
    }
    char dstPortStr[8];
    sprintf(dstPortStr, "%u", dstPort);
    QDateTime current_date_time = QDateTime::currentDateTime();
    ui->receiveEdit->setAlignment(Qt::AlignRight);
    ui->receiveEdit->setTextColor(Qt::green);
    ui->receiveEdit->append(current_date_time.toString("[yyyy-MM-dd hh:mm:ss.zzz]") + "to " + dstAddress.toString() + ":" + dstPortStr);
    ui->receiveEdit->setTextColor(Qt::black);
    ui->receiveEdit->append(txt);
    ui->receiveEdit->append("");
    ui->receiveEdit->setAlignment(Qt::AlignLeft);
}

void NetAssist::on_disconnclient_clicked () {
    int row = ui->clientlist->currentIndex();
    if (row < 0) {
        return;
    }
    QTcpSocket *tc = clients.at(row);
    disconnect(tc, 0 , 0, 0);
    tc->close();
    clients.remove(row);
    ui->clientlist->removeItem(row);
}

void NetAssist::on_clearreceive_clicked () {
    ui->receiveEdit->clear();
}

void NetAssist::on_clearsend_clicked () {
    ui->sendEdit->clear();
}

void NetAssist::on_sendHex_clicked () {
    QString txt = ui->sendEdit->toPlainText();
    QByteArray ba = txt.toUtf8();
    if (ui->sendHex->isChecked()) {
        uchar *s = (uchar*)ba.data();
        int i, size = ba.size();
        char dat[3*size];
        for (i = 0 ; i < size ; i++) {
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
        int i, n = 0;
        for (i = 1 ; i < len ; i+=3) {
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
