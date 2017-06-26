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
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>

#include <ninio/bstream.h>
#include "mcpr/streams.h"

ssize_t bstream_fd_read(struct bstream *stream, void *out, size_t bytes)
{
    return read(*((int *) stream->cookie), out, bytes);
}

ssize_t bstream_fd_write(struct bstream *stream, const void *in, size_t bytes)
{
    return write(*((int *) stream->cookie), in, bytes);
}

void bstream_fd_free(struct bstream *stream)
{
    close(*((int *) stream->cookie));
    free(stream->cookie);
    free(stream);
}

bool bstream_from_fd(struct bstream *stream, int fd)
{
    stream->cookie = malloc(sizeof(fd));
    if(stream->cookie == NULL) { free(stream); return false; }
    *((int *) stream->cookie) = fd;

    stream->read = bstream_fd_read;
    stream->write = bstream_fd_write;
    stream->free = bstream_fd_free;

    return true;
}

ssize_t bstream_read(struct bstream *stream, void *in, size_t bytes)
{
    return stream->read(stream, in, bytes);
}

ssize_t bstream_write(struct bstream *stream, const void *out, size_t bytes)
{
    return stream->write(stream, out, bytes);
}

void bstream_free(struct bstream *stream)
{
    stream->free(stream);
}
