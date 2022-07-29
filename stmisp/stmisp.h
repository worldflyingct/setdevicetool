#ifndef STMISP_H
#define STMISP_H

#include <QWidget>
// 串口相关头文件
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
// 定时器相关头文件
#include <QTimer>

namespace Ui {
class StmIsp;
}

class StmIsp : public QWidget
{
    Q_OBJECT

public:
    explicit StmIsp(QWidget *parent = 0);
    ~StmIsp();
    int GetBtnStatus();

private slots:
    void ReadSerialData();
    void SerialErrorEvent();
    void TimerOutEvent();
    void on_openfile_clicked();
    void on_readchip_clicked();
    void on_readchipmsg_clicked();
    void on_writechip_clicked();
    void on_refresh_clicked();
    void on_erasechip_clicked();
    void on_writeunprotect_clicked();
    void on_writeprotect_clicked();
    void on_readprotect_clicked();
    void on_readunprotect_clicked();
    void on_clearlog_clicked();

private:
    Ui::StmIsp *ui;

    void GetComList();
    int OpenSerial(char *data, qint64 len);
    void CloseSerial();

    QSerialPort serial;
    QTimer timer;
    int btnStatus = 0;

    QString savefilepath;
    uchar serialReadBuff[1024];
    ushort bufflen = 0;
    uchar bin[512*1024];
    uint binlen;
    uchar chipstep = 0;
    uchar retrytime = 0;
    uint addr = 0;
    bool needcheck = 0;
};

#endif // STMISP_H
