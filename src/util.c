#include <string.h>
#include <stdint.h>
#include <time.h>

#include <sys/time.h>

#include "util.h"

enum endianness get_endianness()
{

    // Non portable way, works with GCC. We keep it in here for optimization.
    #if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
        #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            return S_LITTLE_ENDIAN;
        #else
            return S_BIG_ENDIAN;
        #endif
    #endif

    // Portable way to detect endianness below. This unfortainly happens at runtime, hence the non portable
    // optimizations above.
    int n = 1;
    // little endian if true
    return (*(char *)&n == 1) ? S_LITTLE_ENDIAN : S_BIG_ENDIAN;
}

void hton(void *what, size_t n)
{
    // This will most likely be optimized away when optimizations are turned on.
    // normal hton/ntoh functions are slightly faster than our custom one.
    switch(n)
    {
        // case 4: // 32 bits
        //     (*CAST(uint32_t*, what)) = htonl(*CAST(uint32_t*, what));
        // break;
        //
        // case 8: // 64 bits
        //     (*CAST(uint64_t*, what)) = htonll(*CAST(uint64_t*, what));
        // break;
        //
        // case 2: // 16 bits
        //     (*CAST(uint16_t*, what)) = htons(*CAST(uint16_t*, what));
        // break;

        default:
            if(get_endianness() == S_LITTLE_ENDIAN)
            {
                bswap(what, n);
            }
        break;
    }
}

void ntoh(void *what, size_t n)
{
    // This will most likely be optimized away when optimizations are turned on.
    // normal hton/ntoh functions are slightly faster than our custom one.
    switch(n)
    {
        // case 4: // 32 bits
        //     (*CAST(uint32_t*, what)) = ntohl(*CAST(uint32_t*, what));
        // break;
        //
        // case 8: // 64 bits
        //     (*CAST(uint64_t*, what)) = ntohll(*CAST(uint64_t*, what));
        // break;
        //
        // case 2: // 16 bits
        //     (*CAST(uint16_t*, what)) = ntohs(*CAST(uint16_t*, what));
        // break;

        default:
            if(get_endianness() == S_LITTLE_ENDIAN)
            {
                bswap(what, n);
            }
        break;
    }
}

void bswap(void *what, size_t n)
{
    uint8_t tmp[n];
    memcpy(&tmp[0], what, n);

    for(size_t i = 0; i < n; i++)
    {
        ((uint8_t *) what)[i] = tmp[n - 1 - i];
    }
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

void millis_to_timespec(struct timespec *ts, unsigned long ms)
{
    ts->tv_sec = ms / 1000;
    ts->tv_nsec = (ms % 1000) * 1000000;
}

void timespec_to_timeval(struct timeval *tv, const struct timespec *ts)
{
    // TODO implement
}

void timeval_to_timespec(struct timespec *ts, const struct timeval *tv)
{
    // TODO implement
}
