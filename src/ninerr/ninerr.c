#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ninerr/ninerr.h>

struct ninerr *ninerr;

struct ninerr ninerr_out_of_memory_struct;

bool ninerr_init(void)
{
    ninerr_out_of_memory_struct.message = "Out of memory";
    ninerr_out_of_memory_struct.free = NULL;
    ninerr_out_of_memory_struct.child = NULL;
    ninerr_out_of_memory_struct.cause = NULL;
    ninerr_out_of_memory_struct.type = "ninerr_out_of_memory";

    return true;
}

struct ninerr *ninerr_arithmetic_new(void)
{
    struct ninerr *err = ninerr_new("Arithmetic error", false);
    if(err == NULL) return NULL;
    err->type = "ninerr_artihmetic";
    return err;
}

static void ninerr_free(struct ninerr *err)
{
    free(err->message);
    free(err);
}

struct ninerr *ninerr_new(char *message, bool free_message)
{
    struct ninerr *err = malloc(sizeof(struct ninerr));
    if(err == NULL) return NULL;
    err->message = message;
    err->type = "ninerr";
    err->child = NULL;
    err->cause = NULL;
    if(free_message) err->free = ninerr_free; else err->free = NULL;
    return err;
}


void ninerr_finish(void)
{
    ninerr_cleanup_latest();
}

void ninerr_set_err(struct ninerr *err)
{
    ninerr_cleanup_latest();
    ninerr = err;
}

void ninerr_cleanup_latest(void)
{
    if(ninerr != NULL && ninerr->free != NULL) ninerr->free(ninerr);
}

struct ninerr *ninerr_closed_new(char *message, bool free_message)
{
    struct ninerr *err;
    if(message != NULL)
    {
        err = ninerr_new(message, free_message);
    }
    else
    {
        err = ninerr_new("Connection closed.", false);
    }
    if(err == NULL) return NULL;
    err->type = "ninerr_closed";
    return err;
}

struct ninerr *ninerr_wouldblock_new(void)
{
    struct ninerr *err = ninerr_new("Operation would block.", false);
    if(err == NULL) return NULL;
    err->type = "ninerr_wouldblock";
    return err;
}

struct ninerr *ninerr_from_errno(void) // TODO implement other errno values.
{
    switch(errno)
    {
        case ENOMEM:
            return &ninerr_out_of_memory_struct;

        case ECONNRESET:
            return ninerr_closed_new(NULL, false);

        case EAGAIN: // fallthrough
//        case EWOULDBLOCK:
            return ninerr_wouldblock_new();

        default: return ninerr_new(strerror(errno), false);
    }
}
