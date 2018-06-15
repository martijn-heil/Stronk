/*
    MIT License

    Copyright (c) 2017 Martijn Heil

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

#ifndef MCPR_UTIL_H
#define MCPR_UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#define IGNORE(x) \
    DO_GCC_PRAGMA(GCC diagnostic push) \
    DO_GCC_PRAGMA(GCC diagnostic ignored x)

#define END_IGNORE() \
    DO_GCC_PRAGMA(GCC diagnostic pop)



#define htonll(x) ((1==htonl(1)) ? ((uint64_t) x) : ((uint64_t) htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? ((uint64_t) x) : ((uint64_t) ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

#define hton16(x) htons(x)
#define hton32(x) htonl(x)
#define hton64(x) htonll(x)

#define ntoh16(x) ntohs(x)
#define ntoh32(x) ntohl(x)
#define ntoh64(x) ntohll(x)

#ifndef HAVE_ASPRINTF
    static int asprintf(char **strp, const char *fmt, ...)
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
    }
    END_IGNORE()

    #define HAVE_ASPRINTF
#endif

#endif
