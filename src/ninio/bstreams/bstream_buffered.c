#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <ninstd/types.h>
#include <ninstd/mem.h>
#include <ninio/bstream.h>
#include <ninio/ninio.h>
#include <ninerr/ninerr.h>

struct bstream_buffered
{
    struct bstream *stream;
    struct ninio_buffer writebuf;
    usize writebuf_grow_step_size;
    usize write_frame_size;
    u32 reference_count;
};

static bool ensure_write_capacity(struct bstream_buffered *stream, usize capacity)
{
    usize total_required_size = capacity + stream->writebuf.size;
    if (total_required_size <= stream->writebuf.max_size) return true;

    usize new_size = round_usize(total_required_size, stream->writebuf_grow_step_size);
    void *content = realloc(stream->writebuf.content, new_size);
    if (content == NULL)
    {
        ninerr_set_err(ninerr_from_errno());
        return false;
    }

    stream->writebuf.content = content;
    stream->writebuf.max_size = new_size;
    return true;
}

static void bstream_buffered_free(struct bstream *stream)
{
    struct bstream_buffered *private = (struct bstream_buffered *) stream->private;
    bstream_decref(private->stream);
    free(stream);
}

isize bstream_buffered_read_max(struct bstream *stream, void *out, usize bytes)
{
    struct bstream_buffered *private = (struct bstream_buffered *) stream->private;
    return bstream_read_max(private->stream, out, bytes);
}

isize bstream_buffered_peek_max(struct bstream *stream, void *out, usize bytes)
{
    struct bstream_buffered *private = (struct bstream_buffered *) stream->private;
    return bstream_peek_max(private->stream, out, bytes);
}

bool bstream_buffered_is_available(struct bstream *stream, usize amount)
{
    struct bstream_buffered *private = (struct bstream_buffered *) stream->private;
    return bstream_is_available(private->stream, amount);
}

bool bstream_buffered_read(struct bstream *stream, void *out, usize bytes)
{
    struct bstream_buffered *private = (struct bstream_buffered *) stream->private;
    return bstream_read(private->stream, out, bytes);
}

bool bstream_buffered_peek(struct bstream *stream, void *out, usize bytes)
{
    struct bstream_buffered *private = (struct bstream_buffered *) stream->private;
    return bstream_peek(private->stream, out, bytes);
}

isize bstream_buffered_write(struct bstream *stream, const void *in, usize bytes)
{
    struct bstream_buffered *private = (struct bstream_buffered *) stream->private;
    if(!ensure_write_capacity(private, bytes))
    {
        return -1;
    }
    ninio_buffer_append(&(private->writebuf), in, bytes);
    return bytes;
}

isize bstream_buffered_flush(struct bstream *stream)
{
    struct bstream_buffered *private = (struct bstream_buffered *) stream->private;
    struct ninio_buffer *writebuf = &(private->writebuf);

    usize frame_size = private->write_frame_size;
    void *content = writebuf->content;
    usize bytes_remaining = writebuf->size;
    usize total_bytes_written = 0;
    while (bytes_remaining > 0)
    {
        usize bytes_to_write = MIN(bytes_remaining, frame_size);
        isize bytes_written = bstream_write(private->stream, content + total_bytes_written, bytes_to_write);
        total_bytes_written += bytes_written;

        if(bytes_written == -1)
        {
            ninio_buffer_shl(writebuf, total_bytes_written);

            if (ninerr_is_wouldblock(ninerr))
            {
                return bytes_remaining;
            }
            else
            {
                return -1;
            }
        }

        bytes_remaining -= bytes_written;
    }

    ninio_buffer_shl(writebuf, total_bytes_written);
    return 0;
}

static void bstream_buffered_incref(struct bstream *stream)
{
    struct bstream_buffered *private = (struct bstream_buffered *) stream->private;
    private->reference_count++;
}

static void bstream_buffered_decref(struct bstream *stream)
{
    struct bstream_buffered *private = (struct bstream_buffered *) stream->private;
    private->reference_count--;

    if(private->reference_count <= 0)
    {
        bstream_buffered_free(stream);
    }
}

struct bstream *bstream_buffered(
    struct bstream *inner_stream,
    usize write_buf_step_size,
    usize write_buf_initial_size,
    usize write_frame_size)
{
    struct bstream *stream = malloc(sizeof(struct bstream) + sizeof(struct bstream_buffered));
    if(stream == NULL) { ninerr_set_err(ninerr_from_errno()); return NULL; }
    
    stream->private = ((void *) stream) + sizeof(struct bstream);
    struct bstream_buffered *private = (struct bstream_buffered *) stream->private;

    private->reference_count = 1;
    private->stream = inner_stream;
    private->write_frame_size = write_frame_size;
    private->writebuf_grow_step_size = write_buf_step_size;
    private->writebuf.size = 0;
    private->writebuf.max_size = write_buf_initial_size;
    private->writebuf.content = malloc(write_buf_initial_size);
    if (private->writebuf.content == NULL)
    {
        ninerr_set_err(ninerr_from_errno());
        free(stream);
        return NULL;
    }

    stream->is_available = bstream_buffered_is_available;
    stream->peek_max = bstream_buffered_peek_max;
    stream->peek = bstream_buffered_peek;
    stream->read_max = bstream_buffered_read_max;
    stream->read = bstream_buffered_read;
    stream->write = bstream_buffered_write;
    stream->incref = bstream_buffered_incref;
    stream->decref = bstream_buffered_decref;
    stream->flush = bstream_buffered_flush;

    return stream;
}
