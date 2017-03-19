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

#ifndef NINIO_BSTREAM_H
#define NINIO_BSTREAM_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>


struct bstream
{
    void *cookie; // may be NULL
    ssize_t (*read)(struct bstream *stream, void *out, size_t bytes); // May be NULL
    ssize_t (*write)(struct bstream *stream, void *in, size_t bytes); // May be NULL
    void (*free)(struct bstream *stream); // may be NULL
};


bool bstream_from_fd(struct bstream *stream, int fd);


ssize_t bstream_read(struct bstream *stream, void *buf, size_t bytes);
ssize_t bstream_write(struct bstream *stream, const void *buf, size_t bytes);
void bstream_free(struct bstream *stream);

#endif
