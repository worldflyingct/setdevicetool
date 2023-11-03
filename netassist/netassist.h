#ifndef NETASSIST_H
#define NETASSIST_H

#include <QWidget>
// 网络相关头文件
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
// 定时器相关头文件
#include <QTimer>

namespace Ui {
class NetAssist;
}

class NetAssist : public QWidget {
    Q_OBJECT

public:
    explicit NetAssist(QWidget *parent = 0);
    ~NetAssist();
    int GetBtnStatus();

private slots:
    void ReadSocketData();
    void DisconnectSocket();
    void AcceptNewConnect();
    void on_send_clicked();
    void on_protocol_activated(const QString &prot);
    void on_startclose_clicked();
    void on_disconnclient_clicked();
    void on_clearreceive_clicked();
    void on_clearsend_clicked();
    void on_sendHex_clicked();
    void TimerOutEvent();

private:
    Ui::NetAssist *ui;
    QTcpServer tcpServer;
    QTcpSocket tcpClient;
    QUdpSocket udpSocket;
    QTimer timer;
    int btnStatus = 0;
    unsigned char newdata = 1;
    QVector<QTcpSocket*> clients;

    void OpenCloseSocket(int state, QTcpSocket *tc);
};

#endif // NETASSIST_H
