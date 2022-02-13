#ifndef STM32ISP_H
#define STM32ISP_H

#include <QWidget>
// 串口相关头文件
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
// 定时器相关头文件
#include <QTimer>
// cjson库
#include "common/cjson.h"
#include <QFileDialog>

namespace Ui {
class Stm32Isp;
}

class Stm32Isp : public QWidget
{
    Q_OBJECT

public:
    explicit Stm32Isp(QWidget *parent = 0);
    ~Stm32Isp();

private slots:
    void on_openfile_clicked();
    void on_readchip_clicked();
    void on_readchipmsg_clicked();
    void on_writechip_clicked();
    void ReadSerialData();
    void TimerOutEvent();

    void on_refresh_clicked();

    void on_clearlog_clicked();

private:
    Ui::Stm32Isp *ui;

    void GetComList();
    int OpenSerial(char *data, qint64 len);
    void CloseSerial();
    void HandleSerialData(cJSON *json);

    QSerialPort serial;
    QTimer timer;
    int btnStatus = 0;

    unsigned char serialReadBuff[1024];
    unsigned short bufflen = 0;
    unsigned char bin[512*1024];
    unsigned int binlen;
    unsigned char chipstep = 0;
    unsigned char retrytime = 0;
    unsigned long addr = 0;
};

#endif // STM32ISP_H
