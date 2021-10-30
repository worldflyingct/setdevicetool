#ifndef ISP485_H
#define ISP485_H

#include <QWidget>
#include <QButtonGroup>
// 串口相关头文件
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
// 定时器相关头文件
#include <QTimer>

namespace Ui {
class Isp485;
}

class Isp485 : public QWidget
{
    Q_OBJECT

public:
    explicit Isp485(QWidget *parent = 0);
    ~Isp485();

private slots:
    void on_refresh_clicked();
    void ReadSerialData();
    void TimerOutEvent();
    void ModeChanged();

    void on_setconfig_clicked();

private:
    Ui::Isp485 *ui;

    void GetComList();
    int OpenSerial();
    void CloseSerial();
    void HandleSerialData();

    QButtonGroup *groupRadio;
    QSerialPort serial;
    QTimer timer;
    int btnStatus = 0;

    unsigned char serialReadBuff[1024];
    unsigned char bufflen = 0;
};

#endif // ISP485_H
