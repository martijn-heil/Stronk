#ifndef STRONK_UTIL_H
#define STRONK_UTIL_H

#include <stdbool.h>
#include <time.h>

#include <arpa/inet.h>
#include <unistd.h>

#ifdef __GNUC__
    #define DO_PRAGMA(x) _Pragma (#x)
#else
    #define DO_PRAGMA(x)
#endif

#ifdef __GNUC__
    #ifndef likely
        #define likely(x)       __builtin_expect(!!(x), 1)
    #endif

    #ifndef unlikely
        #define unlikely(x)     __builtin_expect(!!(x), 0)
    #endif
#else
    #ifndef likely
        #define likely(x) (x)
    #endif

    #ifndef unlikely
        #define unlikely(x) (x)
    #endif
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

// Returns <0 upon error.
#ifndef HAVE_SECURE_RANDOM
#define HAVE_SECURE_RANDOM
static int secure_random(void *buf, size_t len) {
    int urandomfd = open("/dev/urandom", O_RDONLY);
    if(urandomfd == -1) {
        return -1;
    }

    ssize_t urandomread = read(urandomfd, buf, len);
    if(urandomread == -1) {
        return -1;
    }
    if(urandomread != len) {
        return -1;
    }

    int urandomclose = close(urandomfd);

    return len;
}
#endif

enum endianness {
    S_BIG_ENDIAN,
    S_LITTLE_ENDIAN
};

enum endianness get_endianness();

#endif
