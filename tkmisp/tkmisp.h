#ifndef TKMISP_H
#define TKMISP_H

#include <QWidget>
// 串口相关头文件
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
// 定时器相关头文件
#include <QTimer>

namespace Ui {
class TkmIsp;
}

class TkmIsp : public QWidget
{
    Q_OBJECT

public:
    explicit TkmIsp(QWidget *parent = nullptr);
    ~TkmIsp();
    int GetBtnStatus();

private slots:
    void ReadSerialData();
    void SerialErrorEvent();
    void TimerOutEvent();
    void on_msu0openfile_clicked();
    void on_msu1openfile_clicked();
    void on_writechip_clicked();
    void on_refresh_clicked();
    void on_msu0readchip_clicked();
    void on_msu1readchip_clicked();
    void on_erasechip_clicked();
    void on_readchipmsg_clicked();
    void on_clearlog_clicked();

private:
    Ui::TkmIsp *ui;

    void GetComList();
    int OpenSerial();
    void CloseSerial();

    QSerialPort serial;
    QTimer timer;
    int btnStatus = 0;

    QString savefilepath;
    unsigned char serialReadBuff[1024];
    unsigned short bufflen = 0;
    unsigned char bin0[512*1024];
    unsigned int bin0len;
    unsigned char bin1[512*1024];
    unsigned int bin1len;
    unsigned char chipstep = 0;
    unsigned char retrytime = 0;
    unsigned int addr = 0;
    unsigned int offset = 0;
    bool needcheck = 0;
};

#endif // TKMISP_H
