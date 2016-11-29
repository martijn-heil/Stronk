#ifndef STRONK_UTIL_H
#define STRONK_UTIL_H

#include <stdbool.h>
#include <time.h>

#include <arpa/inet.h>

#ifdef __GNUC__
    #define DO_PRAGMA(x) _Pragma (#x)
#else
    #define DO_PRAGMA(x)
#endif

#ifdef __GNUC__
    #define likely(x)       __builtin_expect(!!(x), 1)
    #define unlikely(x)     __builtin_expect(!!(x), 0)
#else
    #define likely(x) (x)
    #define unlikely(x) (x)
#endif

#ifdef __GNUC__
    #define DO_GCC_PRAGMA(x) DO_PRAGMA(x)
#else
    #define DO_GCC_PRAGMA(x)
#endif

#ifdef __GNUC__
    #define IF_GCC(x) (x)
#else
    #define IF_GCC(x)
#endif

// Use as: TODO("Kill sum more tigers"), also, make sure to use it on it's own line.
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

enum comparison_result {
    GREATER,
    LESS,
    EQUAL,

    ERROR // Shouldnt happen really
};

void timespec_diff(struct timespec *result, const struct timespec *start, const struct timespec *stop);
void timespec_add(struct timespec *result, const struct timespec *t1, const struct timespec *t2);
void timespec_addraw(struct timespec *result, const struct timespec *t1, long sec, long nsec);
enum comparison_result timespec_cmp(const struct timespec *t1, const struct timespec *t2); // Is t1 greater than t2?


enum endianness { // S_* to avoid naming conflicts
    S_BIG_ENDIAN,
    S_LITTLE_ENDIAN
};

enum endianness get_endianness();

#endif
