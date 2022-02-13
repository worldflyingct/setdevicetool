#include "hextobin.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>

typedef struct {
    uchar len;
    uchar type;
    ushort addr;
    uchar data[256];
} BinFarmat;
/********************************************************************************
input:
    c:单个字符('0'~'9' 'a'~'f', 'A'~'F')
output:
    单个字符转化为单个字符
********************************************************************************/
static uchar HexCharToBinChar (char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'z') {
        return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    }
    return 0xff;
}
 
/********************************************************************************
input:
    p: 两个文本字符
output:
    转化为1个字节
********************************************************************************/
static uchar Hex2Bin (unsigned char *p) {
    uchar tmp = 0;
    tmp = HexCharToBinChar(p[0]);
    tmp <<= 4;
    tmp |= HexCharToBinChar(p[1]);
    return tmp;
}
 
/********************************************************************************
input:
    src: hex单行字符串
    p->type: 如果函数返回结果正确，这里就存着转化后的类型
    p->len: 如果函数运行正确，这里就存着转化后的bin数据长度
    p->data: 如果函数运行正确，长度并且不为0，该指针就只想转化后的数据
    p->addr[0]: 如果函数返回结果正确，这里就存着地址的低字节
    p->addr[1]: 如果函数返回结果正确，这里就存着地址的低字节
output:
    返回hex格式流分析的结果
********************************************************************************/
static RESULT_STATUS HexFormatUncode (unsigned char *src, BinFarmat *p) {
    uchar check = 0, tmp[4], binLen;
    ushort hexLen = (ushort)strlen((char*)src);
    ushort num = 0, offset = 0;
    if (hexLen > HEX_MAX_LENGTH) { // 数据内容过长
        return RES_DATA_TOO_LONG;
    }
    if (hexLen < HEX_MIN_LEN) {
        return RES_DATA_TOO_SHORT; // 数据内容过短
    }
    if (src[0] != ':') {
        return RES_NO_COLON; // 没有冒号
	}
    if ((hexLen - 1) % 2 != 0) {
        return RES_LENGTH_ERROR; // hexLen的长度应该为奇数
	}
    binLen = (hexLen - 1) / 2; // bin总数据的长度，包括长度，地址，类型校验等内容
    while (num < 4) {
        offset = (num << 1) + 1;
        tmp[num] = Hex2Bin(src + offset);
        check += tmp[num];
        num++;
    }
    p->len = tmp[0]; // 把解析的这些数据保存到结构体中
    p->addr = tmp[1];
    p->addr <<= 8;
    p->addr += tmp[2];
    p->type = tmp[3];
    while (num < binLen) {
        offset = (num << 1) + 1; // 保存真正的bin格式数据流
        p->data[num - 4] = Hex2Bin(src + offset);
        check += p->data[num - 4];        
        num++;
    }
    if (p->len != binLen - 5) { // 检查hex格式流的长度和数据的长度是否一致
        return RES_LENGTH_ERROR;
	}
    if (check != 0) { // 检查校验是否通过
        return RES_CHECK_ERROR;
	}
    return RES_OK;
}
 
RESULT_STATUS HexToBin (unsigned char *src, unsigned int slength, unsigned char *dest, unsigned int dlength, unsigned long offset, unsigned int *len) {
    unsigned int i, soffset = 0, doffset = 0;
    ushort addr_low;
    ulong addr_hign = 0;
    unsigned char buffer_hex[600];
    BinFarmat gBinFor;
    *len = 0;
    while (soffset < slength) {
        for (i = 0 ; soffset + 1 < slength ; i++) {
            if (src[soffset] == '\r' && src[soffset+1] == '\n') {
                soffset += 2;
                buffer_hex[i] = '\0';
                break;
            }
            buffer_hex[i] = src[soffset];
            soffset++;
        }
        RESULT_STATUS res = HexFormatUncode(buffer_hex, &gBinFor);
        if (res != RES_OK) {
            return res;
        }
        switch (gBinFor.type) {
            case 0: // 数据记录
                addr_low = gBinFor.addr; //数据指针偏移
                doffset = addr_hign + addr_low - offset;
                if (doffset + gBinFor.len > dlength) {
					return RES_WRITE_ERROR;
				}
                memcpy(dest + doffset, gBinFor.data, gBinFor.len);
                doffset += gBinFor.len;
                if (*len < doffset) {
                    *len = doffset;
                }
                break;
            case 1: // 数据结束
                return RES_OK;
            case 2:
                addr_hign = 0;
                for (i = 0 ; i < gBinFor.len ; i++) {
                    addr_hign <<= 8;
                    addr_hign += gBinFor.data[i];
                }
                addr_hign <<= 2;
                break;
            case 4: // 扩展段地址
                addr_hign = 0;
                for (i = 0 ; i < gBinFor.len ; i++) {
                    addr_hign <<= 8;
                    addr_hign += gBinFor.data[i];
                }
                addr_hign <<= 16;
                break;
            case 3:
            case 5:
                break;
            default:
                return RES_TYPE_ERROR;
        }
    }
    return RES_HEX_NO_END;
}
