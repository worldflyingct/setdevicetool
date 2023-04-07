#ifndef SMARTBUILDING22_H
#define SMARTBUILDING22_H

#include <QWidget>
// 串口相关头文件
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
// 定时器相关头文件
#include <QTimer>

namespace Ui {
class Smartbuilding22;
}

class Smartbuilding22 : public QWidget
{
    Q_OBJECT

public:
    explicit Smartbuilding22(QWidget *parent = 0);
    ~Smartbuilding22();
    int GetBtnStatus();

private slots:
    void ReadSerialData();
    void SerialErrorEvent();
    void TimerOutEvent();
    void on_refresh_clicked();
    void on_setconfig_clicked();
    void on_getconfig_clicked();
    void on_devicetype_currentIndexChanged(int index);
    void on_enablesleepnum_stateChanged(int param);

private:
    Ui::Smartbuilding22 *ui;

    void GetComList();
    int OpenSerial();
    void CloseSerial();
    void HandleSerialData(char *data);
    void SetWriteSleepNum(int index, bool checked);

    QSerialPort serial;
    QTimer timer;
    int btnStatus = 0;

    uchar serialReadBuff[1024];
    uchar bin[4*1024];
    ushort bufflen = 0;
    uchar chipstep = 0;
    uchar retrytime = 0;
    uint addr = 0;
    uint offset = 0;
};

#endif // SMARTBUILDING22_H
