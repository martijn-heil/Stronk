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

#ifdef HAVE_FOPENCOOKIE
    static isize fp_from_bstream_write(void *cookie, const char *buf, usize size)
    {
        struct bstream *stream = (struct bstream *) cookie;
        if(!bstream_write(stream, buf, size)) return -1;
        return size;
    }

    static isize fp_from_bstream_read(void *cookie, char *buf, usize size)
    {
        struct bstream *stream = (struct bstream *) cookie;
        if(!bstream_read(stream, buf, size)) return -1;
        return size;
    }

    static isize fp_from_bstream_read_max(void *cookie, char *buf, usize size)
    {
        struct bstream *stream = (struct bstream *) cookie;
        return bstream_read_max(stream, buf, size);
    }

    static i32 fp_from_bstream_close(void *cookie)
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
        else
        {
            return NULL;
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
