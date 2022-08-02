#ifndef RJ45IOTPROGRAM_H
#define RJ45IOTPROGRAM_H

#include <QWidget>
#include <QButtonGroup>
// 串口相关头文件
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
// 定时器相关头文件
#include <QTimer>
// yyjson库
#include "common/yyjson.h"

namespace Ui {
class Rj45IotProgram;
}

class Rj45IotProgram : public QWidget
{
    Q_OBJECT

public:
    explicit Rj45IotProgram(QWidget *parent = 0);
    ~Rj45IotProgram();
    int GetBtnStatus();

private slots:
    void ReadSerialData();
    void SerialErrorEvent();
    void TimerOutEvent();
    void on_refresh_clicked();
    void on_setmode_clicked();
    void on_getmode_clicked();
    void on_dhcp_clicked();
    void on_staticip_clicked();
    void on_rj45mode_clicked();
    void on_wifimode_clicked();
    void on_security_clicked();

private:
    Ui::Rj45IotProgram *ui;

    void GetComList();
    int OpenSerial(char *data, qint64 len);
    void CloseSerial();
    void HandleSerialData(yyjson_val *json);

    QSerialPort serial;
    QTimer timer;
    int btnStatus = 0;

    uchar serialReadBuff[1024];
    ushort bufflen = 0;

    QButtonGroup ipgroup;
    QButtonGroup modegroup;
};

#endif // RJ45IOTPROGRAM_H
