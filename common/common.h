#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

unsigned short crc_calc(unsigned short crc, unsigned char *dat, unsigned short len);
int urldecode(char *url, int urllen, char **protocol, char **host, unsigned short *port, char **path);

#ifdef __cplusplus
}
#endif

#endif // COMMON_H
