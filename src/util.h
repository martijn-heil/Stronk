#ifndef STRONK_UTIL_H
#define STRONK_UTIL_H

#include <arpa/inet.h>

#define DO_PRAGMA(x) _Pragma (#x)

// Use as: TODO("Kill sum more tigers")
#define TODO(x) DO_PRAGMA(message "TODO - " x)


#ifdef USE_GCC_UNION_CAST // gcc specific
    #define CAST(type, x) (((union {__typeof__(x) src; type dst;} *) &(x))->dst)
#else // Proper C11, except that it may break strict aliasing rules.
    #define CAST(type, x) ((type) x)
#endif


#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))


void hton(void *what, size_t n);
void ntoh(void *what, size_t n);
void bswap(void *what, size_t n);

// char* itostr(char *dest, size_t size, int a, int base);



enum endianness {
    S_BIG_ENDIAN,
    S_LITTLE_ENDIAN
};

enum endianness get_endianness();

#endif
