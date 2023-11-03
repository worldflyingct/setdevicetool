#ifndef UARTASSIST_H
#define UARTASSIST_H

#include <QWidget>
// 串口相关头文件
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
// 定时器相关头文件
#include <QTimer>

namespace Ui {
class UartAssist;
}

class UartAssist : public QWidget {
    Q_OBJECT

public:
    explicit UartAssist(QWidget *parent = 0);
    ~UartAssist();
    int GetBtnStatus();

private slots:
    void ReadSerialData();
    void SerialErrorEvent();
    void on_refresh_clicked();
    void on_startclose_clicked();
    void on_send_clicked();
    void on_clearsend_clicked();
    void on_clearreceive_clicked();
    void on_sendHex_clicked();
    void TimerOutEvent();

private:
    Ui::UartAssist *ui;
    void GetComList();
    QSerialPort serial;
    QTimer timer;
    int btnStatus = 0;
    unsigned char newdata = 1;
};

#endif // UARTASSIST_H
