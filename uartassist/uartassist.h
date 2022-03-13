#ifndef UARTASSIST_H
#define UARTASSIST_H

#include <QWidget>
// 串口相关头文件
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

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
    void on_refresh_clicked();
    void on_startclose_clicked();
    void on_send_clicked();
    void on_clearsend_clicked();
    void on_clearreceive_clicked();
    void on_sendHex_clicked();
    void ReadSerialData();

private:
    Ui::UartAssist *ui;
    void GetComList();
    QSerialPort serial;
    int btnStatus = 0;
};

#endif // UARTASSIST_H
