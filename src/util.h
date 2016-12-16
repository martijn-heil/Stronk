#ifndef STRONK_UTIL_H
#define STRONK_UTIL_H

#include <stdbool.h>
#include <time.h>

#include <arpa/inet.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/time.h>

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


#define CAST(type_to, type_from, x) (((union {type_from src; type dst;} *) &(x))->dst)



#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))


void hton(void *what, size_t n);
void ntoh(void *what, size_t n);
void bswap(void *what, size_t n);




void timespec_diff(struct timespec *result, const struct timespec *start, const struct timespec *stop);
void timespec_add(struct timespec *result, const struct timespec *t1, const struct timespec *t2);
void timespec_addraw(struct timespec *result, const struct timespec *t1, long sec, long nsec);
void ms_to_timespec(struct timespec *ts, unsigned long ms);
void timespec_to_timeval(struct timeval *tv, const struct timespec *ts);
void timeval_to_timespec(struct timespec *ts, const struct timeval *tv);


/*
 * Return value:
 * Less than zero if t1 is found to be less than t2.
 * zero if t1 is found to be equal to t2.
 * More than zero if t1 is found to be greater than t2.
 */
int timespec_cmp(const struct timespec *t1, const struct timespec *t2); // Is t1 greater than t2?

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

    close(urandomfd);
    return 0;
}
#endif

enum endianness {
    S_BIG_ENDIAN,
    S_LITTLE_ENDIAN
};

enum endianness get_endianness();

/**
 * Converts a "compressed" UUID in string representation without hyphens,
 * to it's "uncompressed" (official/standardized) representation with hyphens.
 *
 * @param [out] output Output buffer. Should be at least of size 37. Will be NUL terminated by this function.
 * @param [in] input Input buffer. Should be at least of size 32. Does not have to be NUL terminated.
 */
static inline void uuid_decompress_string(char *restrict output, const char *restrict input) {

    // Not using memory copy functions for a good reason, I think the following is a lot clearer than some pointer & copy magic.
    output[0] = input[0];
    output[1] = input[1];
    output[2] = input[2];
    output[3] = input[3];
    output[4] = input[4];
    output[5] = input[5];
    output[6] = input[6];
    output[7] = input[7];

    output[8] = '-';

    output[9] = input[8];
    output[10] = input[9];
    output[11] = input[10];
    output[12] = input[11];

    output[13] = '-';

    output[14] = input[12];
    output[15] = input[13];
    output[16] = input[14];
    output[17] = input[15];

    output[18] = '-';

    output[19] = input[16];
    output[20] = input[17];
    output[21] = input[18];
    output[22] = input[19];

    output[23] = '-';

    output[24] = input[20];
    output[25] = input[21];
    output[26] = input[22];
    output[27] = input[23];
    output[28] = input[24];
    output[29] = input[25];
    output[30] = input[26];
    output[31] = input[27];
    output[32] = input[28];
    output[33] = input[29];
    output[34] = input[30];
    output[35] = input[31];

    output[36] = '\0';
}

static inline void uuid_compress_string(char *restrict output, const char *restrict input) {
    output[0] = input[0];
    output[1] = input[1];
    output[2] = input[2];
    output[3] = input[3];
    output[4] = input[4];
    output[5] = input[5];
    output[6] = input[6];
    output[7] = input[7];

    output[8] = input[9];
    output[9] = input[10];
    output[10] = input[11];
    output[11] = input[12];

    output[12] = input[14];
    output[13] = input[15];
    output[14] = input[16];
    output[15] = input[17];

    output[16] = input[19];
    output[17] = input[20];
    output[18] = input[21];
    output[19] = input[22];

    output[20] = input[24];
    output[21] = input[25];
    output[22] = input[26];
    output[23] = input[27];
    output[24] = input[28];
    output[25] = input[29];
    output[26] = input[30];
    output[27] = input[31];
    output[28] = input[32];
    output[29] = input[33];
    output[30] = input[34];
    output[31] = input[35];
}

#endif
