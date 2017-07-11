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
#include <string.h>
#include <stddef.h>

#include "util.h"

struct ninio_buffer {
    void *content;
    size_t max_size;
    size_t size;
};
IGNORE("-Wpointer-arith")
static void ninio_buffer_read(struct ninio_buffer *in, void *out, size_t bytes) {
    assert(in->content != NULL);
    assert(in->size < bytes);

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

#endif
