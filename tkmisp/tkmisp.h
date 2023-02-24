#ifndef TKMISP_H
#define TKMISP_H

#include <QWidget>
// 串口相关头文件
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
// 定时器相关头文件
#include <QTimer>
// 网络控制相关头文件
#include <QUdpSocket>

namespace Ui {
class TkmIsp;
}

class TkmIsp : public QWidget
{
    Q_OBJECT

public:
    explicit TkmIsp(QWidget *parent = nullptr);
    ~TkmIsp();
    int GetBtnStatus();

private slots:
    void ReadSocketData();
    void ReadSerialData();
    void SerialErrorEvent();
    void TimerOutEvent();
    void on_msu0openfile_clicked();
    void on_msu1openfile_clicked();
    void on_writechip_clicked();
    void on_refresh_clicked();
    void on_msu0readchip_clicked();
    void on_msu1readchip_clicked();
    void on_erasechip_clicked();
    void on_readchipmsg_clicked();
    void on_clearlog_clicked();
    void on_netctl_clicked();

private:
    Ui::TkmIsp *ui;

    void GetComList();
    int OpenSerial();
    void CloseSerial();
    void SendSocketData(char *dat, int len);

    QSerialPort serial;
    QTimer timer;
    QUdpSocket udpSocket;
    QHostAddress srcAddress;
    quint16 srcPort;
    int btnStatus = 0;

    QString savefilepath;
    uchar serialReadBuff[1024];
    ushort bufflen = 0;
    uchar bin0[256*1024];
    uint bin0len;
    uchar bin1[96*1024];
    uint bin1len;
    uchar chipstep = 0;
    uchar retrytime = 0;
    uint addr = 0;
    uint offset = 0;
    bool needcheck = 0;
    uchar netctlStatus = 0;
    char failmsg[4];
    char successmsg[7];
    uint eraseStart;
    uint eraseEnd;
};

#endif // TKMISP_H
