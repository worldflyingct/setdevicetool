#ifndef ISPIOCONTROLLER_H
#define ISPIOCONTROLLER_H

#include <QWidget>
// 串口相关头文件
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
// 定时器相关头文件
#include <QTimer>

namespace Ui {
class IspIoController;
}

class IspIoController : public QWidget
{
    Q_OBJECT

public:
    explicit IspIoController(QWidget *parent = 0);
    ~IspIoController();

private slots:
    void on_refresh_clicked();
    void on_setmqtt_clicked();
    void on_readmqtt_clicked();
    void ReadSerialData();
    void TimerOutEvent();

private:
    Ui::IspIoController *ui;

    void GetComList();
    int OpenSerial();
    void CloseSerial();
    void HandleSerialData();

    QSerialPort serial;
    QTimer timer;
    int btnStatus = 0;

    unsigned char serialReadBuff[1024];
    unsigned char bufflen = 0;
};

#endif // ISPIOCONTROLLER_H