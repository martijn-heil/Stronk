#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <features.h>

#if defined(__GNU_LIBRARY__) || defined(__GLIBC__)
    #include <execinfo.h>
#endif

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
    struct ninerr *err = ninerr_new("Arithmetic error");
    if(err == NULL) return NULL;
    err->type = "ninerr_artihmetic";
    return err;
}

int ninerr_print(const struct ninerr *err)
{
    return ninerr_fprint(stderr, err);
}

int ninerr_fprint(FILE *fp, const struct ninerr *err)
{
    if(err == NULL) return 0;

    if(err->message != NULL && err->stacktrace != NULL)
    {
        return fprintf(fp, "%s: %s\n%s", err->type, err->message, err->stacktrace);
    }
    else if(err->stacktrace != NULL)
    {
        return fprintf(fp, "%s:\n%s", err->type, err->stacktrace);
    }
    else if(err->message != NULL)
    {
        return fprintf(fp, "%s: %s\n", err->type, err->message);
    }
    return -1;
}

static void ninerr_free(struct ninerr *err)
{
    if(err->stacktrace != NULL) free(err->stacktrace);
    free(err);
}

struct ninerr *ninerr_vnew(const char *fmt, va_list ap)
{
    size_t required_message_length = 0;
    if(fmt != NULL)
    {
        char tmpbuf;
        int result = vsnprintf(&tmpbuf, 1, fmt, ap);
        if(result != -1) required_message_length = result;
    }

    size_t required_size = sizeof(struct ninerr);
    if(required_message_length > 0) required_size += required_message_length + 1;

    struct ninerr *err = malloc(required_size);
    if(err == NULL) { return NULL; }


    if(required_message_length > 0)
    {
        err->message = (char *) (err + sizeof(struct ninerr));
        int result = vsprintf(err->message, fmt, ap);
        if(result == -1) err->message = NULL;
    }
    else
    {
        err->message = NULL;
    }

    err->type = "ninerr";
    err->child = NULL;
    err->cause = NULL;
    err->free = ninerr_free;

    #if defined(__GNU_LIBRARY__) || defined(__GLIBC__)
        #define MAX_STACK_LEVELS 50
        void *buffer[MAX_STACK_LEVELS];
        int levels = backtrace(buffer, MAX_STACK_LEVELS);

        //backtrace_symbols_fd(buffer, levels, 2);
        char **symbols = backtrace_symbols(buffer, levels);
        if(symbols == NULL) { err->stacktrace = NULL; return err; }
        size_t total_size = 0;
        for(unsigned int i = 0; i < (unsigned int) levels; i++)
        {
            total_size += strlen(symbols[i]) + 4; // \n + a + t +' '
        }

        err->stacktrace = malloc(total_size);
        if(err->stacktrace == NULL) { free(symbols); return err; }

        char *ptr = err->stacktrace;
        for(unsigned int i = 0; i < (unsigned int) levels; i++)
        {
            ptr[0] = 'a';
            ptr[1] = 't';
            ptr[2] = ' ';
            size_t len = strlen(symbols[i]);
            memcpy(ptr + 3, symbols[i], len);
            ptr[3 + len] = '\n';
            ptr += 3 + len + 1;
        }
        *ptr = '\0';
        free(symbols);
    #else
        err->stacktrace = NULL;
    #endif

    return err;
}

struct ninerr *ninerr_new(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    struct ninerr *result = ninerr_vnew(fmt, ap);
    va_end(ap);
    return result;
}


void ninerr_finish(void)
{
    ninerr_cleanup_latest();
}

void ninerr_set_err(struct ninerr *err)
{
    ninerr_cleanup_latest();
    if(err == NULL) // temporary hack, not sure if ninerr_set_err(NULL should be allowed)
    {
        ninerr = ninerr_new(NULL);
    }
    else
    {
        ninerr = err;
    }
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
        err = ninerr_new("Connection closed.");
    }
    if(err == NULL) return NULL;
    err->type = "ninerr_closed";
    return err;
}

struct ninerr *ninerr_wouldblock_new(void)
{
    struct ninerr *err = ninerr_new("Operation would block.");
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

        default: return ninerr_new(strerror(errno));
    }
}
