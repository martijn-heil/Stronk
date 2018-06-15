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

#ifndef NINIO_H
#define NINIO_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <limits.h>

#include "util.h"

struct ninio_buffer {
    void *content;
    size_t max_size;
    size_t size;
};
IGNORE("-Wpointer-arith")
static void ninio_buffer_read(struct ninio_buffer *in, void *out, size_t bytes) {
    assert(in->content != NULL);
    assert(in->size >= bytes);

    memcpy(out, in->content, bytes);
    if(in->size - bytes > 0)
    {
        memmove(in->content, in->content + bytes, in->size - bytes);
    }
    else
    {
        in->size = 0;
    }
}
END_IGNORE()

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
