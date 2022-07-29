#ifndef __HEXTOBIN_H__
#define __HEXTOBIN_H__

#ifdef __cplusplus
extern "C"
{
#endif

//typedef unsigned char uchar;
//typedef unsigned short ushort;
//typedef unsigned long ulong;
 
#define HEX_MAX_LENGTH  521
#define HEX_MIN_LEN     11
 
typedef enum {
    RES_OK = 0,         //正确
    RES_DATA_TOO_LONG,  //数据太长
    RES_DATA_TOO_SHORT, //数据太短
    RES_NO_COLON,       //无标号
    RES_TYPE_ERROR,     //类型出错，或许不存在
    RES_LENGTH_ERROR,   //数据长度字段和内容的总长度不对应
    RES_CHECK_ERROR,    //校验错误
    RES_WRITE_ERROR,    //写数据出错
    RES_HEX_NO_END      //hex文件没有结束符
} RESULT_STATUS;

unsigned char HexCharToBinChar(char c);
// src为hex的二进制文件
// slength为hex的二进制文件大小
// dest为目标bin的缓存
// dlength目标bin的缓存的大小
// offset为flash写入的偏移量，stm32默认为0x08000000
// len为获取新生成的bin文件实际大小
RESULT_STATUS HexToBin(unsigned char *src, unsigned int slength, unsigned char *dest, unsigned int dlength, unsigned long offset, unsigned int *len);

#ifdef __cplusplus
}
#endif

#endif
