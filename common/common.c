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

int urldecode (char *url, int urllen, char **protocol, char **serveraddr, unsigned short *port) {
    *protocol = url;
    unsigned short p = 0;
    char state = 0;
    for (int i = 0 ; i < urllen ; i++) {
        if (state == 0 && url[i] == ':') {
            if (url[i+1] != '/' || url[i+2] != '/') {
                return 1;
            }
            url[i] = '\0';
            i += 2;
            *serveraddr = url + i + 1;
            state = 1;
        } else if (state == 1 && url[i] == ':') {
            url[i] = '\0';
            state = 2;
        } else if (state == 2) {
            if ('0' <= url[i] && url[i] <= '9') {
                p = 10*p + url[i] - '0';
            } else if (url[i] == '\0') {
                *port = p;
                return 0;
            } else {
                return 2;
            }
        }
    }
    if (state != 2) {
        return 3;
    }
    *port = p;
    return 0;
}
