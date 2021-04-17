#include "dialog.h"
#include "./ui_dialog.h"
#include <windows.h>

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;
}

unsigned short crc_calc (unsigned short crc, unsigned char *dat, unsigned short len) {
    unsigned short i;
    for (i = 0 ; i < len ; i++) { // 循环计算每个数据
        unsigned char j;
        crc ^= dat[i]; // 将八位数据与crc寄存器亦或，然后存入crc寄存器
        for (j = 0; j < 8; j++) { // 循环计算数据的
            if (crc & 0x0001) { // 判断右移出的是不是1，如果是1则与多项式进行异或。
                crc >>= 1; // 将数据右移一位
                crc ^= 0xa001; // 与上面的多项式进行异或
            } else { // 如果不是1，则直接移出
                crc >>= 1; // 将数据右移一位
            }
        }
    }
    return crc;
}

void Dialog::on_setnet_clicked()
{
    unsigned char serialdata[14];
    const char *ip = ui->ip->text().toStdString().c_str();
    const char *mask = ui->mask->text().toStdString().c_str();
    const char *gateway = ui->gateway->text().toStdString().c_str();
    int res;
    res = sscanf(ip, "%d.%d.%d.%d", &serialdata[0], &serialdata[1], &serialdata[2], &serialdata[3]);
    if (res != 4) {
        ui->tips->setText("ip输入错误");
        return;
    }
    res = sscanf(mask, "%d.%d.%d.%d", &serialdata[4], &serialdata[5], &serialdata[6], &serialdata[7]);
    if (res != 4) {
        ui->tips->setText("掩码输入错误");
        return;
    }
    res = sscanf(gateway, "%d.%d.%d.%d", &serialdata[8], &serialdata[9], &serialdata[10], &serialdata[11]);
    if (res != 4) {
        ui->tips->setText("网关输入错误");
        return;
    }
    for (int i = 0 ; i < 4 ; i++) {
        if (serialdata[i] > 255) {
            ui->tips->setText("ip输入错误");
            return;
        }
        if (serialdata[i+4] > 255) {
            ui->tips->setText("掩码输入错误");
            return;
        }
        if (serialdata[i+8] > 255) {
            ui->tips->setText("网关输入错误");
            return;
        }
    }
    unsigned short crc = 0xffff;
    crc = crc_calc(crc, serialdata, 12);
    serialdata[12] = crc;
    serialdata[13] = crc>>8;
    char com[12];
    sprintf(com, "\\\\.\\%s", ui->com->currentText().toStdString().c_str());
    HANDLE hCom = CreateFileA(com, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if(hCom == INVALID_HANDLE_VALUE) {
        ui->tips->setText("串口打开失败");
        return;
    }
    DCB myDCB;
    myDCB.BaudRate = CBR_115200;   // 设置波特率115200
    myDCB.fBinary = TRUE; // 设置二进制模式，此处必须设置TRUE
    myDCB.fParity = TRUE; // 支持奇偶校验
    myDCB.fOutxCtsFlow = FALSE;  // No CTS output flow control
    myDCB.fOutxDsrFlow = FALSE;  // No DSR output flow control
    myDCB.fDtrControl = DTR_CONTROL_DISABLE; // No DTR flow control
    myDCB.fDsrSensitivity = FALSE; // DSR sensitivity
    myDCB.fTXContinueOnXoff = TRUE; // XOFF continues Tx
    myDCB.fOutX = FALSE;     // No XON/XOFF out flow control
    myDCB.fInX = FALSE;        // No XON/XOFF in flow control
    myDCB.fErrorChar = FALSE;    // Disable error replacement
    myDCB.fNull = FALSE;  // Disable null stripping
    myDCB.fRtsControl = RTS_CONTROL_DISABLE;   //No RTS flow control
    myDCB.fAbortOnError = FALSE;  // 当串口发生错误，并不终止串口读写
    myDCB.ByteSize = 8;   // 数据位,范围:4-8
    myDCB.Parity = NOPARITY; // 校验模式
    myDCB.StopBits = 0;   // 1位停止位
    SetCommState(hCom, &myDCB);
    SetupComm(hCom,1024,1024); //缓冲区大小
    COMMTIMEOUTS TimeOuts;
    TimeOuts.ReadIntervalTimeout = 2;
    TimeOuts.ReadTotalTimeoutMultiplier = 2;
    TimeOuts.ReadTotalTimeoutConstant = 5;
    // 设定写超时
    TimeOuts.WriteTotalTimeoutMultiplier = 50;
    TimeOuts.WriteTotalTimeoutConstant = 200;
    SetCommTimeouts(hCom, &TimeOuts); //设置超时
    PurgeComm(hCom, PURGE_TXCLEAR|PURGE_RXCLEAR);
    DWORD sendlen;
    WriteFile(hCom, serialdata, sizeof(serialdata), &sendlen, NULL);
    CloseHandle(hCom);
    ui->tips->setText("设置成功");
}
