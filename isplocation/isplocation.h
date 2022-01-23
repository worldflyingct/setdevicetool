#ifndef ISPLOCATION_H
#define ISPLOCATION_H

#include <QWidget>
// 串口相关头文件
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
// 定时器相关头文件
#include <QTimer>
// cjson库
#include "common/cjson.h"

namespace Ui {
class IspLocation;
}

class IspLocation : public QWidget {
    Q_OBJECT

public:
    explicit IspLocation(QWidget *parent = 0);
    ~IspLocation();

private slots:
    void on_refresh_clicked();
    void on_setconfig_clicked();
    void on_getconfig_clicked();
    void ReadSerialData();
    void TimerOutEvent();

private:
    Ui::IspLocation *ui;

    void GetComList();
    int OpenSerial(char *data, qint64 len);
    void CloseSerial();
    void HandleSerialData(cJSON *json);

    QSerialPort serial;
    QTimer timer;
    int btnStatus = 0;

    unsigned char serialReadBuff[1024];
    unsigned short bufflen = 0;
};

#endif // ISPLOCATION_H
