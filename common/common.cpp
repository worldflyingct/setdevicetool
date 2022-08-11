#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include "common.h"
#include <QFileDialog>

ushort COMMON::crc_calc (ushort crc, uchar *dat, ushort len) {
    ushort i;
    for (i = 0 ; i < len ; i++) { // 循环计算每个数据
        uchar j;
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

int COMMON::urldecode (const char *url, int urllen, char *protocol, char *host, ushort *port, char *path) {
    uint p = 0;
    int offset = 0, state = 0;
    for (int i = 0 ; i < urllen ; i++) {
        if (state == 0 && url[i] == ':') {
            if (i+2 < urllen && (url[i+1] != '/' || url[i+2] != '/')) {
                return -1;
            }
            if (protocol) {
                int tlen = i - offset;
                memcpy(protocol, url, tlen);
                protocol[tlen] = '\0';
            }
            i += 3;
            offset = i;
            state = 1;
        } else if (state == 1 && (url[i] == ':' || url[i] == '/' || url[i] == '\0')) {
            if (host) {
                int tlen = i - offset;
                memcpy(host, url + offset, tlen);
                host[tlen] = '\0';
            }
            offset = i;
            if (url[i] == ':') {
                p = 0;
                state = 2;
            } else if (url[i] == '/') {
                state = 3;
            } else if (url[i] == '\0') {
                return 0;
            }
        } else if (state == 2) {
            if ('0' <= url[i] && url[i] <= '9') {
                p = 10*p + url[i] - '0';
                if (p > 65536) {
                    return -2;
                }
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
                return -3;
            }
        } else if (state == 3 && url[i] == '\0') {
            if (path) {
                int tlen = i - offset;
                memcpy(path, url + offset, tlen);
                path[tlen] = '\0';
            }
            return 0;
        }
    }
    if (state == 1) {
        if (host) {
            int tlen = urllen - offset;
            memcpy(host, url + offset, tlen);
            host[tlen] = '\0';
        }
    } else if (state == 2) {
        if (port) {
            *port = p;
        }
    } else if (state == 3) {
        if (path) {
            int tlen = urllen - offset;
            memcpy(path, url + offset, tlen);
            path[tlen] = '\0';
        }
    }
    return 0;
}

int COMMON::filewrite (QString savefilepath, char *bin, uint addr, char *buff, uchar openmodeflag) {
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

int COMMON::TKM_HexToBin(uchar *src, uint slength, uchar *dest, uint dlength, uint *len) {
    uint offset = 0;
    for (uint i = 0 ; i < slength ; i += 10) {
        if (src[i+8] != 0x0d || src[i+9] != 0x0a) {
            return -1; // hex格式错误
        }
        if (offset + 4 > dlength) {
            return -2; // dest太小
        }
        dest[offset] = src[i+6] <= '9' ? src[i+6]-'0' : src[i+6]+10-'A';
        dest[offset] *= 16;
        dest[offset] += src[i+7] <= '9' ? src[i+7]-'0' : src[i+7]+10-'A';
        offset++;
        dest[offset] = src[i+4] <= '9' ? src[i+4]-'0' : src[i+4]+10-'A';
        dest[offset] *= 16;
        dest[offset] += src[i+5] <= '9' ? src[i+5]-'0' : src[i+5]+10-'A';
        offset++;
        dest[offset] = src[i+2] <= '9' ? src[i+2]-'0' : src[i+2]+10-'A';
        dest[offset] *= 16;
        dest[offset] += src[i+3] <= '9' ? src[i+3]-'0' : src[i+3]+10-'A';
        offset++;
        dest[offset] = src[i] <= '9' ? src[i]-'0' : src[i]+10-'A';
        dest[offset] *= 16;
        dest[offset] += src[i+1] <= '9' ? src[i+1]-'0' : src[i+1]+10-'A';
        offset++;
    }
    *len = offset;
    return 0;
}

bool COMMON::CheckValidIp (const char *ip) {
    int count = 0;
    ushort p = 0;
    for (int i = 0 ; ip[i] != '\0' ; i++) {
        if (ip[i] < '0' || ip[i] > '9') {
            if (ip[i] == '.' ) {
                p = 0;
                count++;
                continue;
            }
            return false;
        }
        p = 10*p + ip[i] - '0';
        if (p > 255) {
            return false;
        }
    }
    if (count != 3) {
        return false;
    }
    return true;
}

bool COMMON::CheckValidTelephone (const char *telephone) {
    int i;
    for (i = 0 ; telephone[i] != '\0' ; i++) {
        if (telephone[i] < '0' || telephone[i] > '9') {
            return false;
        }
    }
    if (telephone[0] != '1' || i != 11) {
        return false;
    }
    return true;
}
