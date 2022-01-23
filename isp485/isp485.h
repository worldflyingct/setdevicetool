#ifndef ISP485_H
#define ISP485_H

#include <QWidget>
#include <QButtonGroup>
// 串口相关头文件
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
// 定时器相关头文件
#include <QTimer>
// cjson库
#include "common/cjson.h"

namespace Ui {
class Isp485;
}

class Isp485 : public QWidget {
    Q_OBJECT

public:
    explicit Isp485(QWidget *parent = 0);
    ~Isp485();

private slots:
    void on_refresh_clicked();
    void on_setconfig_clicked();
    void on_getconfig_clicked();
    void ReadSerialData();
    void TimerOutEvent();
    void ModeChanged();

private:
    Ui::Isp485 *ui;

    void GetComList();
    int OpenSerial(char *data, qint64 len);
    void CloseSerial();
    void HandleSerialData(cJSON *json);

    QSerialPort serial;
    QTimer timer;
    int btnStatus = 0;

    unsigned char serialReadBuff[1024];
    unsigned short bufflen = 0;

    QButtonGroup *groupRadio;
};

#endif // ISP485_H
