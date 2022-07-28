#ifndef COMMON_H
#define COMMON_H

#include <QString>

class COMMON {
    public:
        static unsigned short crc_calc(unsigned short crc, unsigned char *dat, unsigned short len);
        static int urldecode(char *url, int urllen, char **protocol, char **host, unsigned short *port, char **path);
        static int filewrite(QString savefilepath, char *bin, unsigned int addr, char *buff, unsigned char openmodeflag);
        static int TKM_HexToBin(unsigned char *src, unsigned int slength, unsigned char *dest, unsigned int dlength, unsigned int *len);
};

#endif // COMMON_H
