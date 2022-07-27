#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include "common.h"
#include <QFileDialog>

unsigned short COMMON::crc_calc (unsigned short crc, unsigned char *dat, unsigned short len) {
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

int COMMON::urldecode (char *url, int urllen, char **protocol, char **host, unsigned short *port, char **path) {
    unsigned short i, offset, p = 0, state = 0;
    if (protocol) {
        *protocol = NULL;
    }
    if (host) {
        *host = NULL;
    }
    if (port) {
        *port = 0;
    }
    if (path) {
        *path = NULL;
    }
    if (url[urllen-1] != '\0') {
        return 1;
    }
    for (i = 0 ; i < urllen ; i++) {
        if (state == 0 && url[i] == ':') {
            if (i+2 < urllen && (url[i+1] != '/' || url[i+2] != '/')) {
                return 2;
            }
            if (protocol) {
                int tlen = i;
                char *t = (char*)malloc(tlen+1);
                if (t == NULL) {
                    return 3;
                }
                memcpy(t, url, i);
                t[i] = '\0';
                *protocol = t;
            }
            i += 3;
            offset = i;
            state = 1;
        } else if (state == 1) {
            if (url[i] == ':' || url[i] == '/' || url[i] == '\0') {
                if (host) {
                    int tlen = i-offset;
                    char *t = (char*)malloc(tlen+1);
                    if (t == NULL) {
                        return 5;
                    }
                    memcpy(t, url + offset, tlen);
                    t[tlen] = '\0';
                    *host = t;
                }
                offset = i;
                if (url[i] == ':') {
                    state = 2;
                } else if (url[i] == '/') {
                    state = 3;
                } else if (url[i] == '\0') {
                    return 0;
                }
            }
        } else if (state == 2) {
            if ('0' <= url[i] && url[i] <= '9') {
                p = 10*p + url[i] - '0';
            } else if (url[i] == '/' || url[i] == '\0') {
                if (port) {
                    *port = p;
                }
                offset = i;
                if (url[i] == '/') {
                    state = 3;
                } else if (url[i] == '\0') {
                    return 0;
                }
            } else {
                return 6;
            }
        } else if (state == 3) {
            if (url[i] == '\0') {
                if (path) {
                    int tlen = i-offset;
                    char *t = (char*)malloc(tlen+1);
                    if (t == NULL) {
                        return 7;
                    }
                    memcpy(t, url + offset, tlen);
                    t[tlen] = '\0';
                    *path = t;
                }
                offset = i;
                return 0;
            }
        }
    }
    return 0;
}

int COMMON::filewrite (QString savefilepath, char *bin, unsigned int addr, char *buff, unsigned char openmodeflag) {
    int res = 0;
    QFile file(savefilepath);
    QIODevice::OpenMode flag = openmodeflag > 0 ? QIODevice::WriteOnly | QIODevice::Append : QIODevice::WriteOnly;
    if (!file.open(flag)) {
        if (buff != NULL) {
            sprintf(buff, "文件打开失败");
        }
        res = -1;
    } else {
        if (file.write(bin, addr) < addr) {
            if (buff != NULL) {
                sprintf(buff, "文件写入不完整");
            }
            res = -2;
        } else {
            if (buff != NULL) {
                sprintf(buff, "镜像保存成功");
            }
        }
        file.close();
    }
    return res;
}
