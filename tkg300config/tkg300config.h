#ifndef TKG300CONFIG_H
#define TKG300CONFIG_H

#include <QWidget>
#include <QButtonGroup>
// 串口相关头文件
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
// 定时器相关头文件
#include <QTimer>

namespace Ui {
class Tkg300Config;
}

class Tkg300Config : public QWidget
{
    Q_OBJECT

public:
    explicit Tkg300Config(QWidget *parent = 0);
    ~Tkg300Config();
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
    Ui::Tkg300Config *ui;

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

#endif // TKG300CONFIG_H
