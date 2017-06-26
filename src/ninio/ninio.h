#ifndef NINIO_H
#define NINIO_H

#include <stddef.h>

struct ninio_buffer {
    void *content;
    size_t max_size;
    size_t size;
};

#endif
