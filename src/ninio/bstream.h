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
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>

#include <ninstd/types.h>


struct bstream
{
    void *private; // may be NULL
    bool (*read)(struct bstream *stream, void *out, usize bytes); // May be NULL
    isize (*read_max)(struct bstream *stream, void *out, usize maxbytes); // May be NULL
    bool (*peek)(struct bstream *stream, void *out, usize bytes); // May be NULL
    isize (*peek_max)(struct bstream *stream, void *out, usize maxbytes); // May be NULL
    isize (*write)(struct bstream *stream, const void *in, usize bytes); // May be NULL
    void (*incref)(struct bstream *stream); // may be NULL, if either incref or decref is provided, the opposite function must be provided too.
    void (*decref)(struct bstream *stream); // may be NULL, if either incref or decref is provided, the opposite function must be provided too.
    bool (*is_available)(struct bstream *stream, usize amount); // may be NULL
    isize (*flush)(struct bstream *stream); // may be NULL
};

/*
    Returns a bstream for the given file descriptor.
    Provided functionality:
    read
    read_max
    write
    incref
    decref
    is_available? TODO
*/
struct bstream *bstream_from_fd(i32 fd);
struct bstream *bstream_buffered(
    struct bstream *inner_stream,
    usize write_buf_step_size,
    usize write_buf_initial_size,
    usize write_frame_size);

bool bstream_is_available(struct bstream *stream, usize amount);
isize bstream_read_max(struct bstream *stream, void *buf, usize maxbytes);
bool bstream_read(struct bstream *stream, void *buf, usize bytes);
isize bstream_write(struct bstream *stream, const void *buf, usize bytes);
bool bstream_peek(struct bstream *stream, void *out, usize bytes);
isize bstream_peek_max(struct bstream *stream, void *out, usize maxbytes);
isize bstream_flush(struct bstream *stream);

// If the following two functions are called on a bstream where incref/decref is NULL, no operation will be performed.
void bstream_incref(struct bstream *stream);
void bstream_decref(struct bstream *stream);


int bstream_vprintf(struct bstream *stream, const char *fmt, va_list ap);
int bstream_printf(struct bstream *stream, const char *fmt, ...);

#ifdef HAVE_FOPENCOOKIE
    #define HAVE_FP_FROM_BSTREAM
    FILE *fp_from_bstream(struct bstream *stream);
#endif

void bstream_init_black_hole(struct bstream *stream);
void bstream_open_memory(struct bstream *out, void *buf, usize maxlen, char *mode); // TODO implement
bool bstream_memory_writer(struct bstream *stream, void **buf, usize initial_size); // TODO implement

#endif
