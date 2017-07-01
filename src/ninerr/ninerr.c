#include <stdlib.h>
#include <errno.h>
#include <ninerr/ninerr.h>

struct ninerr *ninerr = NULL;

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

struct ninerr *ninerr_artihmetic_new(void)
{
    struct ninerr *err = ninerr_new("Arithmetic error", false);
    err->type = "ninerr_artihmetic";
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
    if(ninerr != NULL) ninerr->free(ninerr);
}

struct ninerr *ninerr_from_errno(void) // TODO implement other errno values.
{
    switch(errno)
    {
        case ENOMEM:
        {
            return &ninerr_out_of_memory_struct;
        }

        default:
        {
            return NULL;
        }
    }
}
