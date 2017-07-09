#ifndef NINERR_H
#define NINERR_H

#include <stdbool.h>

// TODO maybe line & file at which error occurred?
struct ninerr
{
    char *message; // may be NULL
    void *child; // may be NULL
    void *cause; // may be NULL
    const char *type; // may not be NULL.

    void (*free)(struct ninerr *err); // may be NULL.
};
extern struct ninerr *ninerr; // may be NULL, usually you don't want to assign to this directly, use ninerr_set_err instead.
void ninerr_set_err(struct ninerr *err); // err may be NULL.
void ninerr_cleanup_latest(void);
bool ninerr_init(void);
void ninerr_finish(void);
struct ninerr *ninerr_from_errno(void); // may return NULL
struct ninerr *ninerr_new(char *message, bool free_message);
struct ninerr *ninerr_arithmetic_new(void);



extern struct ninerr ninerr_out_of_memory_struct;


struct ninerr_io
{
    struct ninerr *super;
};

struct ninerr_file_not_found
{
    struct ninerr_io_err *super;
};

struct ninerr_closed
{
    struct ninerr_io_err *super;
};

#endif
