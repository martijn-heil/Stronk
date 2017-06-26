#ifndef NINIO_H
#define NINIO_H

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

struct ninio_buffer {
    void *content;
    size_t max_size;
    size_t size;
};

static void ninio_buffer_read(struct ninio_buffer *in, void *out, size_t bytes) {
    if(in->content == NULL) return;
    if(in->size < bytes) abort();

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

#endif
