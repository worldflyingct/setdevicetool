#ifndef COMMON_H
#define COMMON_H

#include <QString>

class COMMON {
    public:
        static ushort crc_calc(ushort crc, uchar *dat, ushort len);
        static int urldecode(const char *url, int urllen, char *protocol, char *host, ushort *port, char *path);
        static int filewrite(QString savefilepath, char *bin, uint addr, char *buff, uchar openmodeflag);
        static int TKM_HexToBin(uchar *src, uint slength, uchar *dest, uint dlength, uint *len);
        static bool CheckValidIp(const char *ip);
        static bool CheckValidTelephone(const char *telephone);
};

#endif // COMMON_H
