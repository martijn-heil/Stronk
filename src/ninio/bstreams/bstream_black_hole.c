/*
    MIT License

    Copyright (c) 2024 Martijn Heil

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

#include <stdio.h>

#include <ninstd/types.h>
#include <ninio/bstream.h>

static isize bstream_black_hole_write(struct bstream *stream, const void *in, usize size)
{
    return size;
}

void bstream_init_black_hole(struct bstream *stream)
{
    stream->is_available = NULL;
    stream->peek_max = NULL;
    stream->peek = NULL;
    stream->read_max = NULL;
    stream->read = NULL;
    stream->write = bstream_black_hole_write;
    stream->incref = NULL;
    stream->decref = NULL;
    stream->flush = NULL;
}
