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


struct bstream
{
    void *private; // may be NULL
    bool (*read)(struct bstream *stream, void *out, size_t bytes); // May be NULL
    size_t (*read_max)(struct bstream *stream, void *out, size_t maxbytes); // May be NULL
    bool (*peek)(struct bstream *stream, void *out, size_t bytes); // May be NULL
    size_t (*peek_max)(struct bstream *stream, void *out, size_t maxbytes); // May be NULL
    bool (*write)(struct bstream *stream, const void *in, size_t bytes); // May be NULL
    void (*incref)(struct bstream *stream); // may be NULL
    void (*decref)(struct bstream *stream); // may be NULL
    bool (*is_available)(struct bstream *stream, size_t amount); // may be NULL
};

/*
    Returns a bstream for the given file descriptor.
    All functionality will be available, none of the functions will be NULL.
*/
bool bstream_from_fd(struct bstream *stream, int fd);

bool bstream_is_available(struct bstream *stream, size_t amount);
size_t bstream_read_max(struct bstream *stream, void *buf, size_t maxbytes);
bool bstream_read(struct bstream *stream, void *buf, size_t bytes);
bool bstream_write(struct bstream *stream, const void *buf, size_t bytes);
void bstream_peek(struct bstream *stream, void *out, size_t bytes);
void bstream_incref(struct bstream *stream);
void bstream_decref(struct bstream *stream);

#endif
