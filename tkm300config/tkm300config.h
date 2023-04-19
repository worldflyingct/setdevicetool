#ifndef TKM300CONFIG_H
#define TKM300CONFIG_H

#include <QWidget>
#include <QButtonGroup>
// 串口相关头文件
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
// 定时器相关头文件
#include <QTimer>

namespace Ui {
class Tkm300Config;
}

class Tkm300Config : public QWidget
{
    Q_OBJECT

public:
    explicit Tkm300Config(QWidget *parent = 0);
    ~Tkm300Config();
    int GetBtnStatus();

private slots:
    void ReadSerialData();
    void SerialErrorEvent();
    void TimerOutEvent();
    void on_refresh_clicked();
    void on_setconfig_clicked();
    void on_getconfig_clicked();

    void on_staticip_clicked();

    void on_dhcp_clicked();

private:
    Ui::Tkm300Config *ui;

    void GetComList();
    int OpenSerial();
    void CloseSerial();

    QSerialPort serial;
    QTimer timer;
    int btnStatus = 0;

    uchar serialReadBuff[1024];
    ushort bufflen = 0;
    uchar chipstep = 0;

    QButtonGroup ipgroup;
};

#endif // TKM300CONFIG_H
