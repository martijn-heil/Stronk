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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>

#include <ninerr/ninerr.h>
#include <ninio/bstream.h>
#include <ninio/ninio.h>
#include "mcpr/streams.h"

struct fd_bstream
{
    struct ninio_buffer buf;
    int fd;
    unsigned int reference_count;
};

static void bstream_fd_free(struct bstream *stream);

static bool attempt_fill_up_buffer(struct ninio_buffer *buf, int fd, size_t size) {
    if(buf->size < size)
    {
        size_t remaining = size - buf->size;

        if(buf->max_size < buf->size + remaining)
        {
            void *tmp = realloc(buf->content, buf->size + remaining);
            if(tmp == NULL)
            {
                size_t possible = buf->max_size - buf->size;
                ssize_t result = read(fd, buf->content + buf->size, possible);
                if(result < 0)
                {
                    if(errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        return true;
                    }
                    else
                    {
                        ninerr_set_err(ninerr_from_errno());
                        return false;
                    }
                }
                buf->size += result;
                return true;
            }

            buf->content = tmp;
            buf->max_size = buf->size + remaining;
        }

        ssize_t result = read(fd, buf->content + buf->size, remaining);
        if(result < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return true;
            }
            else
            {
                ninerr_set_err(ninerr_from_errno());
                return false;
            }
        }
        buf->size += result;
        return true;
    }
    return true;
}

ssize_t bstream_fd_read_max(struct bstream *stream, void *out, size_t bytes)
{
    struct fd_bstream *private = (struct fd_bstream *) stream->private;
    if(!attempt_fill_up_buffer(&(private->buf), private->fd, bytes)) { return -1; }

    if(private->buf.size >= bytes)
    {
        ninio_buffer_read(&(private->buf), out, bytes);
        return bytes;
    }
    else
    {
        size_t possible = private->buf.size;
        ninio_buffer_read(&(private->buf), out, possible);
        return possible;
    }
}

ssize_t bstream_fd_peek_max(struct bstream *stream, void *out, size_t bytes)
{
    struct fd_bstream *private = (struct fd_bstream *) stream->private;
    attempt_fill_up_buffer(&(private->buf), private->fd, bytes);

    if(private->buf.size >= bytes)
    {
        memcpy(out, private->buf.content, bytes);
        return bytes;
    }
    else
    {
        memcpy(out, private->buf.content, private->buf.size);
        return private->buf.size;
    }
}

bool bstream_fd_is_available(struct bstream *stream, size_t amount)
{
    struct fd_bstream *private = (struct fd_bstream *) stream->private;
    attempt_fill_up_buffer(&(private->buf), private->fd, amount);
    return private->buf.size >= amount;
}

bool bstream_fd_read(struct bstream *stream, void *out, size_t bytes)
{
    struct fd_bstream *private = (struct fd_bstream *) stream->private;
    attempt_fill_up_buffer(&(private->buf), private->fd, bytes);
    if(private->buf.size >= bytes)
    {
        ninio_buffer_read(&(private->buf), out, bytes);
        return true;
    }
    else
    {
        ninerr_set_err(ninerr_new("Not enough bytes available.", false));
        return false;
    }
}

bool bstream_fd_peek(struct bstream *stream, void *out, size_t bytes)
{
    struct fd_bstream *private = (struct fd_bstream *) stream->private;
    attempt_fill_up_buffer(&(private->buf), private->fd, bytes);
    if(private->buf.size >= bytes)
    {
        memcpy(out, private->buf.content, bytes);
        return true;
    }
    else
    {
        return false;
    }
}

bool bstream_fd_write(struct bstream *stream, const void *in, size_t bytes)
{
    struct fd_bstream *private = (struct fd_bstream *) stream->private;
    return write(private->fd, in, bytes);
}

static void bstream_fd_incref(struct bstream *stream)
{
    struct fd_bstream *private = (struct fd_bstream *) stream->private;
    private->reference_count++;
}

static void bstream_fd_decref(struct bstream *stream)
{
    struct fd_bstream *private = (struct fd_bstream *) stream->private;
    private->reference_count--;

    if(private->reference_count <= 0)
    {
        bstream_fd_free(stream);
    }
}

static void bstream_fd_free(struct bstream *stream)
{
    struct fd_bstream *private = (struct fd_bstream *) stream->private;
    free(stream->private);
    free(stream);
}

bool bstream_read(struct bstream *stream, void *in, size_t bytes)
{
    return stream->read(stream, in, bytes);
}

bool bstream_write(struct bstream *stream, const void *out, size_t bytes)
{
    return stream->write(stream, out, bytes);
}

ssize_t bstream_read_max(struct bstream *stream, void *buf, size_t maxbytes)
{
    return stream->read_max(stream, buf, maxbytes);
}

struct bstream *bstream_from_fd(int fd)
{
    struct bstream *stream = malloc(sizeof(struct bstream));
    if(stream == NULL) { ninerr_set_err(ninerr_from_errno()); return NULL; }

    stream->private = malloc(sizeof(struct fd_bstream));
    if(stream->private == NULL) { ninerr_set_err(ninerr_from_errno()); free(stream); return NULL; }
    struct fd_bstream *private = (struct fd_bstream *) stream->private;
    private->fd = fd;
    private->reference_count = 1;
    private->buf.size = 0;
    private->buf.max_size = 0;
    private->buf.content = NULL;

    stream->is_available = bstream_fd_is_available;
    stream->peek_max = bstream_fd_peek_max;
    stream->peek = bstream_fd_peek;
    stream->read_max = bstream_fd_read_max;
    stream->read = bstream_fd_read;
    stream->write = bstream_fd_write;
    stream->incref = bstream_fd_incref;
    stream->decref = bstream_fd_decref;

    return stream;
}

void bstream_incref(struct bstream *stream)
{
    if(stream->incref != NULL) stream->incref(stream);
}

void bstream_decref(struct bstream *stream)
{
    if(stream->decref != NULL) stream->decref(stream);
}


int bstream_vprintf(struct bstream *stream, const char *fmt, va_list ap)
{
    char *buf;
    va_list ap2;
    va_copy(ap2, ap);
    int result = vasprintf(&buf, fmt, ap2);
    va_end(ap2);
    if(result < 0) return -1;
    if((unsigned int) result > SIZE_MAX) { free(buf); return -1; }
    if(!bstream_write(stream, buf, result)) { free(buf); return -1; }
    free(buf);
    return result;
}

int bstream_printf(struct bstream *stream, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int result = bstream_vprintf(stream, fmt, ap);
    va_end(ap);
    return result;
}

#ifdef HAVE_FOPENCOOKIE
    static ssize_t fp_from_bstream_write(void *cookie, const char *buf, size_t size)
    {
        struct bstream *stream = (struct bstream *) cookie;
        if(!bstream_write(stream, buf, size)) return -1;
        return size;
    }

    static ssize_t fp_from_bstream_read(void *cookie, char *buf, size_t size)
    {
        struct bstream *stream = (struct bstream *) cookie;
        if(!bstream_read(stream, buf, size)) return -1;
        return size;
    }

    static ssize_t fp_from_bstream_read_max(void *cookie, char *buf, size_t size)
    {
        struct bstream *stream = (struct bstream *) cookie;
        return bstream_read_max(stream, buf, size);
    }

    static int fp_from_bstream_close(void *cookie)
    {
        bstream_decref((struct bstream *) cookie);
        return 0;
    }

    FILE *fp_from_bstream(struct bstream *stream)
    {
        const char *mode;
        if(stream->write != NULL && (stream->read != NULL || stream->read_max != NULL))
        {
            mode = "r+";
        }
        else if(stream->write != NULL)
        {
            mode = "w";
        }
        else if(stream->read != NULL || stream->read_max != NULL)
        {
            mode = "r";
        }

        cookie_io_functions_t funcs;
        funcs.seek = NULL;
        if(stream->write != NULL) funcs.write = fp_from_bstream_write; else funcs.write = NULL;
        if(stream->read != NULL) funcs.read = fp_from_bstream_read; else if(stream->read_max != NULL) funcs.read = fp_from_bstream_read_max; else funcs.read = NULL;
        funcs.close = fp_from_bstream_close;
        FILE *new_fp = fopencookie(stream, mode, funcs);
        if(new_fp == NULL) return NULL;
        if(setvbuf(new_fp, NULL, _IOLBF, 0) != 0) // Ensure line buffering.
        {
            fclose(new_fp);
            return NULL;
        }
        bstream_incref(stream);
        return new_fp;
    }
#endif
