/*
    MIT License

    Copyright (c) 2016 Martijn Heil

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef STRONK_UTIL_H
#define STRONK_UTIL_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>

#include <arpa/inet.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "warnings.h"



#ifndef DO_PRAGMA
    #ifdef __GNUC__
        #define DO_PRAGMA(x) _Pragma (#x)
    #else
        #define DO_PRAGMA(x)
    #endif
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
    #ifndef DO_GCC_PRAGMA
        #define DO_GCC_PRAGMA(x) DO_PRAGMA(x)
    #endif
#else
    #ifndef DO_GCC_PRAGMA
        #define DO_GCC_PRAGMA(x)
    #endif
#endif

#ifdef __GNUC__
    #ifndef IF_GCC
        #define IF_GCC(x) (x)
    #endif
#else
    #ifndef IF_GCC
        #define IF_GCC(x)
    #endif
#endif

#ifndef thread_local
# if __STDC_VERSION__ >= 201112 && !defined __STDC_NO_THREADS__
#  define thread_local _Thread_local
# elif defined _WIN32 && ( \
       defined _MSC_VER || \
       defined __ICL || \
       defined __DMC__ || \
       defined __BORLANDC__ )
#  define thread_local __declspec(thread)
/* note that ICC (linux) and Clang are covered by __GNUC__ */
# elif defined __GNUC__ || \
       defined __SUNPRO_C || \
       defined __xlC__
#  define thread_local __thread
# else
#  error "Cannot define thread_local"
# endif
#endif



#define CAST(type_to, type_from, x) (((union {type_from src; type dst;} *) &(x))->dst)


#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))


#define hton16(x) htons(x)
#define hton32(x) htonl(x)
#define hton64(x) htonll(x)

#define ntoh16(x) ntohs(x)
#define ntoh32(x) ntohl(x)
#define ntoh64(x) ntohll(x)


void bswap(void *what, size_t n);




void timespec_diff(struct timespec *result, const struct timespec *start, const struct timespec *stop);
void timespec_add(struct timespec *result, const struct timespec *t1, const struct timespec *t2); // All timespecs can alias eachother
void timespec_addraw(struct timespec *result, const struct timespec *t1, long sec, long nsec);
void ms_to_timespec(struct timespec *ts, unsigned long long ms);
void timespec_to_timeval(struct timeval *tv, const struct timespec *ts);
void timeval_to_timespec(struct timespec *ts, const struct timeval *tv);


/*
 * Return value:
 * Less than zero if t1 is found to be less than t2.
 * zero if t1 is found to be equal to t2.
 * More than zero if t1 is found to be greater than t2.
 */
int timespec_cmp(const struct timespec *t1, const struct timespec *t2); // Is t1 greater than t2?

#ifndef HAVE_ASPRINTF
    /*static int asprintf(char **strp, const char *fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        int result = vasprintf(strp, fmt, ap);
        va_end(ap);
        return result;
    }

    IGNORE("-Wtype-limits")
    static int vasprintf(char **strp, const char *fmt, va_list ap)
    {
        char tmp;
        va_list ap2;
        va_copy(ap2, ap);
        int bytes_required = vsnprintf(&tmp, 1, fmt, ap2);
        va_end(ap2);
        if(bytes_required < 0) return -1;
        if((unsigned int) bytes_required > SIZE_MAX) return -1;
        bytes_required++; // For the NUL byte

        char *buf = malloc(bytes_required);
        if(buf == NULL) return -1;

        va_list ap3;
        va_copy(ap3, ap);
        int result2 = vsprintf(buf, fmt, ap3);
        va_end(ap3);
        if(result2 < 0) { free(buf); return -1; }
        *strp = buf;
        return result2;
    }*/
    END_IGNORE()

    #define HAVE_ASPRINTF
#endif

// Returns <0 upon error.
#ifndef HAVE_SECURE_RANDOM
#define HAVE_SECURE_RANDOM
static ssize_t secure_random(void *buf, size_t len) {
    int urandomfd = open("/dev/urandom", O_RDONLY);
    if(urandomfd < 0) {
        return -1;
    }

    ssize_t urandomread = read(urandomfd, buf, len);
    if(urandomread < 0) {
        return -1;
    }
    if((size_t) urandomread != len) {
        return -1;
    }

    close(urandomfd);

    return len;
}
#endif

// Stupid E prefix to prevent name clashes
enum endianness {
    E_BIG_ENDIAN,
    E_LITTLE_ENDIAN
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
