#ifndef ISP485_H
#define ISP485_H

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
class Isp485;
}

class Isp485 : public QWidget {
    Q_OBJECT

public:
    explicit Isp485(QWidget *parent = 0);
    ~Isp485();
    int GetBtnStatus();

private slots:
    void ReadSerialData();
    void SerialErrorEvent();
    void TimerOutEvent();
    void on_refresh_clicked();
    void on_setconfig_clicked();
    void on_getconfig_clicked();
    void on_mqttradio_clicked();
    void on_otherradio_clicked();

private:
    Ui::Isp485 *ui;

    void GetComList();
    int OpenSerial(char *data, qint64 len);
    void CloseSerial();
    void HandleSerialData(yyjson_val *json);

    QSerialPort serial;
    QTimer timer;
    int btnStatus = 0;

    uchar serialReadBuff[1024];
    ushort bufflen = 0;
};

#endif // ISP485_H
