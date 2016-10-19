#ifndef STRONK_UTIL_H
#define STRONK_UTIL_H

#include <arpa/inet.h>


#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))


void hton(void *what, size_t n);
void ntoh(void *what, size_t n);
void bswap(void *what, size_t n);

enum endianness {
    S_BIG_ENDIAN,
    S_LITTLE_ENDIAN
};

enum endianness get_endianness();

#endif
