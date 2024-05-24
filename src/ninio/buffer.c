#include <string.h>
#include <assert.h>
#include <ninstd/types.h>
#include <ninio/ninio.h>

IGNORE("-Wpointer-arith")
void ninio_buffer_read(struct ninio_buffer *in, void *out, usize bytes) {
    assert(in->content != NULL);
    assert(in->size >= bytes);

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

void ninio_buffer_append(struct ninio_buffer *out, void const *restrict in, usize n)
{
    memcpy(out->content + out->size, in, n);
    out->size += n;
}

void ninio_buffer_append_byte(struct ninio_buffer *out, u8 in)
{
    *((char *) (out->content + out->size)) = in;
    out->size++;
}

void ninio_buffer_shl(struct ninio_buffer *buffer, usize n)
{
    if (n == 0) return;
    memmove(buffer->content, buffer->content + n, buffer->size - n);
    buffer->size = buffer->size - n;
}
END_IGNORE()
