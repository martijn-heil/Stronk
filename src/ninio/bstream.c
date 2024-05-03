#include <stdio.h>

#include <ninstd/types.h>
#include <ninio/bstream.h>

bool bstream_read(struct bstream *stream, void *in, usize bytes)
{
    return stream->read(stream, in, bytes);
}

isize bstream_write(struct bstream *stream, const void *out, usize bytes)
{
    return stream->write(stream, out, bytes);
}

ssize_t bstream_read_max(struct bstream *stream, void *buf, usize maxbytes)
{
    return stream->read_max(stream, buf, maxbytes);
}

void bstream_incref(struct bstream *stream)
{
    if(stream->incref != NULL) stream->incref(stream);
}

void bstream_decref(struct bstream *stream)
{
    if(stream->decref != NULL) stream->decref(stream);
}

bool bstream_peek(struct bstream *stream, void *out, usize bytes)
{
    return stream->peek(stream, out, bytes);
}

isize bstream_peek_max(struct bstream *stream, void *out, usize maxbytes)
{
    return stream->peek_max(stream, out, maxbytes);
}

bool bstream_is_available(struct bstream *stream, usize amount)
{
    return stream->is_available(stream, amount);
}

isize bstream_flush(struct bstream *stream)
{
    if (stream->flush != NULL)
    {
        return stream->flush(stream);
    } else return 0;
}

int bstream_vprintf(struct bstream *stream, const char *fmt, va_list ap)
{
    char *buf;
    va_list ap2;
    va_copy(ap2, ap);
    int result = vasprintf(&buf, fmt, ap2);
    va_end(ap2);
    if(result < 0) return -1;
    if((u32) result > SIZE_MAX) { free(buf); return -1; }
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

