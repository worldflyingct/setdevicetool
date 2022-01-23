#ifndef ISPIOTPROGRAM_H
#define ISPIOTPROGRAM_H

#include <QWidget>
// 串口相关头文件
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
// 定时器相关头文件
#include <QTimer>
// cjson库
#include "common/cjson.h"

namespace Ui {
class IspIotProgram;
}

class IspIotProgram : public QWidget {
    Q_OBJECT

public:
    explicit IspIotProgram(QWidget *parent = 0);
    ~IspIotProgram();

private slots:
    void on_refresh_clicked();
    void on_setmode_clicked();
    void on_readmode_clicked();
    void ReadSerialData();
    void TimerOutEvent();

private:
    Ui::IspIotProgram *ui;

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

#endif // ISPIOTPROGRAM_H
