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
    unsigned char serialReadBuff[1024];
    unsigned short bufflen = 0;
    unsigned char bin[512*1024];
    unsigned int binlen;
    unsigned char chipstep = 0;
    unsigned char retrytime = 0;
    unsigned int addr = 0;
    bool needcheck = 0;
};

#endif // STMISP_H
