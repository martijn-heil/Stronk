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

#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>

#include <sys/time.h>

#include "util.h"

enum endianness get_endianness()
{

    // Non portable way, works with GCC. We keep it in here for optimization.
    #if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
        #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            return E_LITTLE_ENDIAN;
        #else
            return E_BIG_ENDIAN;
        #endif
    #endif

    // Portable way to detect endianness below. This happens at runtime, hence the non portable
    // optimizations above.
    int n = 1;
    // little endian if true
    return (*(char *)&n == 1) ? E_LITTLE_ENDIAN : E_BIG_ENDIAN;
}

// char* itostr(char *dest, size_t size, int a, int base) {
//   static char buffer[sizeof a * CHAR_BIT + 1 + 1];
//   static const char digits[36] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
//
//   if (base < 2 || base > 36) {
//     return NULL;
//   }
//
//   char* p = &buffer[sizeof(buffer) - 1];
//   *p = '\0';
//
//   int an = a < 0 ? a : -a;
//
//   // Works with negative `int`
//   do {
//     *(--p) = digits[-(an % base)];
//     an /= base;
//   } while (an);
//
//   if (a < 0) {
//     *(--p) = '-';
//   }
//
//   size_t size_used = &buffer[sizeof(buffer)] - p;
//   if (size_used > size) {
//     return NULL;
//   }
//   return memcpy(dest, p, size_used);
// }

void timespec_diff(struct timespec *result, const struct timespec *start, const struct timespec *stop)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0)
    {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    }
    else
    {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}

void timespec_add(struct timespec *result, const struct timespec *t1, const struct timespec *t2)
{
    result->tv_sec = t2->tv_sec + t1->tv_sec;
    result->tv_nsec = t2->tv_nsec + t1->tv_nsec;

    if (result->tv_nsec >= 1000000000)
    {
        result->tv_nsec -= 1000000000;
        result->tv_sec++;
    }
}

void timespec_addraw(struct timespec *result, const struct timespec *t, long sec, long nsec)
{
    result->tv_sec = sec + t->tv_sec;
    result->tv_nsec = nsec + t->tv_nsec;

    if (result->tv_nsec >= 1000000000)
    {
        result->tv_nsec -= 1000000000;
        result->tv_sec++;
    }
}

int timespec_cmp(const struct timespec *t1, const struct timespec *t2) {
    if(t1->tv_sec > t2->tv_sec || (t1->tv_sec == t2->tv_sec && t1->tv_nsec > t2->tv_nsec))
    {
        return 1;
    }
    else if(t1->tv_sec == t2->tv_sec && t1->tv_nsec == t2->tv_nsec)
    {
        return 0;
    }
    else if(t1->tv_sec < t2->tv_sec || (t1->tv_sec == t2->tv_sec && t1->tv_nsec < t2->tv_nsec))
    {
        return -1;
    }

    return -1; // This shouldn't actually ever be reached.. But else the compiler will complain.
}

void ms_to_timespec(struct timespec *ts, unsigned long long ms)
{
    ts->tv_sec = ms / 1000;
    ts->tv_nsec = (ms % 1000) * 1000000;
}

// void timespec_to_timeval(struct timeval *tv, const struct timespec *ts)
// {
//     // TODO implement
// }
//
// void timeval_to_timespec(struct timespec *ts, const struct timeval *tv)
// {
//     // TODO implement
// }

int msleep(i64 msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}
