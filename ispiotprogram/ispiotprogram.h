#ifndef ISPIOTPROGRAM_H
#define ISPIOTPROGRAM_H

#include <QWidget>
// 串口相关头文件
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
// 定时器相关头文件
#include <QTimer>
// yyjson库
#include "common/yyjson.h"

namespace Ui {
class IspIotProgram;
}

class IspIotProgram : public QWidget {
    Q_OBJECT

public:
    explicit IspIotProgram(QWidget *parent = 0);
    ~IspIotProgram();
    int GetBtnStatus();

private slots:
    void ReadSerialData();
    void SerialErrorEvent();
    void TimerOutEvent();
    void on_refresh_clicked();
    void on_setmode_clicked();
    void on_getmode_clicked();

private:
    Ui::IspIotProgram *ui;

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

#endif // ISPIOTPROGRAM_H
