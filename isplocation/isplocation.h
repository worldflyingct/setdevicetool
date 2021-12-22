#ifndef ISPLOCATION_H
#define ISPLOCATION_H

#include <QWidget>
// 串口相关头文件
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

namespace Ui {
class IspLocation;
}

class IspLocation : public QWidget
{
    Q_OBJECT

public:
    explicit IspLocation(QWidget *parent = 0);
    ~IspLocation();

private slots:
    void on_refresh_clicked();
    void on_setconfig_clicked();
    void on_getconfig_clicked();

private:
    Ui::IspLocation *ui;
    void GetComList();
};

#endif // ISPLOCATION_H
