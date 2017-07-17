/*
    MIT License

    Copyright (c) 2017 Martijn Heil

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

#ifndef NINERR_H
#define NINERR_H

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#ifndef thread_local
# if __STDC_VERSION__ >= 201112 && !defined __STDC_NO_THREADS__
#  define thread_local _Thread_local
# elif defined _WIN32 && ( \
       defined _MSC_VER || \
       defined __ICL || \
       defined __DMC__ || \
       defined __BORLANDC__ )
#  define thread_local __declspec(thread)
/* note that ICC (linux) and Clang are covered by __GNUC__ */
# elif defined __GNUC__ || \
       defined __SUNPRO_C || \
       defined __xlC__
#  define thread_local __thread
# else
#  error "Cannot define thread_local"
# endif
#endif

// TODO maybe line & file at which error occurred?
struct ninerr
{
    char *message; // may be NULL
    char *stacktrace; // may be NULL.
    void *child; // may be NULL
    void *cause; // may be NULL
    const char *type; // may not be NULL.

    void (*free)(struct ninerr *err); // may be NULL.
};
extern thread_local struct ninerr *ninerr; // may be NULL, usually you don't want to assign to this directly, use ninerr_set_err instead.
void ninerr_set_err(struct ninerr *err); // err may be NULL.
void ninerr_cleanup_latest(void);
bool ninerr_init(void);
void ninerr_finish(void);
struct ninerr *ninerr_from_errno(void); // may return NULL
struct ninerr *ninerr_new(const char *fmt, ...);
struct ninerr *ninerr_arithmetic_new(void);
int ninerr_print(const struct ninerr *err);
int ninerr_fprint(FILE *fp, const struct ninerr *err);



extern struct ninerr ninerr_out_of_memory_struct;


struct ninerr_io
{
    struct ninerr *super;
};

struct ninerr_file_not_found
{
    struct ninerr_io *super;
};

struct ninerr *ninerr_closed_new(char *message, bool free_message); // message can be NULL
struct ninerr *ninerr_wouldblock_new(void);

#endif
