#include "common.h"

unsigned short crc_calc (unsigned short crc, unsigned char *dat, unsigned short len)
{
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
