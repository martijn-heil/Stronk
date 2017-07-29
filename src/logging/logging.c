/*
    MIT License

    Copyright (c) 2016 Martijn Heil

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

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#include <sys/types.h>
#include <features.h>

#include <zlog.h>

#include <ninio/bstream.h>

#include "logging/logging.h"
#include "../warnings.h"


/*
    <quote>

    In the GNU C Library, stdin, stdout, and stderr are normal variables which you can set just like any others. For example,
    to redirect the standard output to a file, you could do:
        fclose (stdout);
        stdout = fopen ("standard-output-file", "w");

    </quote>

    Source: http://www.gnu.org/software/libc/manual/html_node/Standard-Streams.html
*/

#if defined(__GNU_LIBRARY__) && 0 || defined(__GLIBC__) && 0
    #define STANDARD_STREAMS_ASSIGNABLE 1
#else
    #define STANDARD_STREAMS_ASSIGNABLE 0
#endif

#if STANDARD_STREAMS_ASSIGNABLE && defined(HAVE_FOPENCOOKIE)
    #define DO_REDIRECTION
#endif

#undef DO_REDIRECTION // temporary

static zlog_category_t *_zc;


IGNORE("-Wreturn-type")
zlog_level log_level_to_zlog(enum log_level level)
{
    switch(level)
    {
        case LOG_LEVEL_DEBUG: return ZLOG_LEVEL_DEBUG;
        case LOG_LEVEL_NOTICE: return ZLOG_LEVEL_NOTICE;
        case LOG_LEVEL_INFO: return ZLOG_LEVEL_INFO;
        case LOG_LEVEL_WARN: return ZLOG_LEVEL_WARN;
        case LOG_LEVEL_ERROR: return ZLOG_LEVEL_ERROR;
        case LOG_LEVEL_FATAL: return ZLOG_LEVEL_FATAL;
    }
}
END_IGNORE()

FILE *fp_fatal;
FILE *fp_error;
FILE *fp_warn;
FILE *fp_notice;
FILE *fp_info;
FILE *fp_debug;

struct bstream *bstream_fatal;
struct bstream *bstream_error;
struct bstream *bstream_warn;
struct bstream *bstream_notice;
struct bstream *bstream_info;
struct bstream *bstream_debug;

struct logger *nlogger;
static struct logger nlogger_raw;

static void vnlog(const char *filename, size_t filename_len, const char *func, size_t func_len, int line, enum log_level level, const char *fmt, va_list ap)
{
    vzlog(_zc, filename, filename_len, __func__, func_len, line, log_level_to_zlog(level), fmt, ap);
}

bool bstreamlog_write(struct bstream *stream, const void *buf, size_t size)
{
    enum log_level level = *((enum log_level *) stream->private);

    char *new_buf;
    bool free_new_buf = false;
    if(size <= 256)
    {
        char buf[size + 1];
        new_buf = buf;
    }
    else
    {
        char *new_buf = malloc(size + 1);
        if(new_buf == NULL) return false;
        free_new_buf = true;
    }

    memcpy(new_buf, buf, size);

    // Remove newline at the end, if there is one.
    if(new_buf[size - 1] == '\n')
    {
        new_buf[size - 1] = '\0';
    }
    else
    {
        new_buf[size] = '\0';
    }

    logger_write(nlogger, "null", sizeof("null")-1, "null", sizeof("null")-1, 0, level, "%s", new_buf);
    if(free_new_buf) free(new_buf);
    return true;
}


int logging_init(void)
{
    int zlog_status = zlog_init("/etc/zlog.conf"); // TODO maybe configuration for other paths.
    if(zlog_status)
    {
        fprintf(stderr, "Could not initialize zlog with /etc/zlog.conf (%s ?)\n", strerror(errno));
        return -1;
    }

    _zc = zlog_get_category("stronk");
    if(!_zc)
    {
        fprintf(stderr, "Could not get category 'stronk' for zlog from /etc/zlog.conf, if you have not yet defined this category, define it.");
        zlog_fini();
        return -1;
    }

    nlogger = &nlogger_raw;
    nlogger->vwrite = vnlog;


    nlog_info("Setting application locale to make sure we use UTF-8..");
    if(setlocale(LC_ALL, "") == NULL) // important, make sure we can use UTF-8.
    {
        nlog_warn("Could not set application locale to make sure we use UTF-8.");
    }

    bstream_fatal = malloc(sizeof(struct bstream) + sizeof(enum log_level));
    if(bstream_fatal == NULL) { zlog_fini(); nlog_fatal("Could not allocate memory. (%s)", strerror(errno)); return -1; }
    enum log_level *bstream_fatal_level = (enum log_level *) (((void *) bstream_fatal) + sizeof(struct bstream));
    *bstream_fatal_level = LOG_LEVEL_FATAL;
    bstream_fatal->private = bstream_fatal_level;
    bstream_fatal->write = bstreamlog_write;
    bstream_fatal->read = NULL;
    bstream_fatal->read_max = NULL;
    bstream_fatal->peek = NULL;
    bstream_fatal->peek_max = NULL;
    bstream_fatal->incref = NULL;
    bstream_fatal->decref = NULL;
    bstream_fatal->is_available = NULL;

    bstream_error = malloc(sizeof(struct bstream) + sizeof(enum log_level));
    if(bstream_error == NULL) { free(bstream_fatal); zlog_fini(); nlog_fatal("Could not allocate memory. (%s)", strerror(errno)); return -1; }
    enum log_level *bstream_error_level = (enum log_level *) (((void *) bstream_error) + sizeof(struct bstream));
    *bstream_error_level = LOG_LEVEL_ERROR;
    bstream_error->private = bstream_error_level;
    bstream_error->write = bstreamlog_write;
    bstream_error->read = NULL;
    bstream_error->read_max = NULL;
    bstream_error->peek = NULL;
    bstream_error->peek_max = NULL;
    bstream_error->incref = NULL;
    bstream_error->decref = NULL;
    bstream_error->is_available = NULL;

    bstream_warn = malloc(sizeof(struct bstream) + sizeof(enum log_level));
    if(bstream_warn == NULL) { free(bstream_warn); zlog_fini(); nlog_fatal("Could not allocate memory. (%s)", strerror(errno)); return -1; }
    enum log_level *bstream_warn_level = (enum log_level *) (((void *) bstream_warn) + sizeof(struct bstream));
    *bstream_error_level = LOG_LEVEL_WARN;
    bstream_warn->private = bstream_warn_level;
    bstream_warn->write = bstreamlog_write;
    bstream_warn->read = NULL;
    bstream_warn->read_max = NULL;
    bstream_warn->peek = NULL;
    bstream_warn->peek_max = NULL;
    bstream_warn->incref = NULL;
    bstream_warn->decref = NULL;
    bstream_warn->is_available = NULL;

    bstream_notice = malloc(sizeof(struct bstream) + sizeof(enum log_level));
    if(bstream_notice == NULL) { free(bstream_fatal); free(bstream_error); zlog_fini(); nlog_fatal("Could not allocate memory. (%s)", strerror(errno)); return -1; }
    enum log_level *bstream_notice_level = (enum log_level *) (((void *) bstream_notice) + sizeof(struct bstream));
    *bstream_notice_level = LOG_LEVEL_NOTICE;
    bstream_notice->private = bstream_notice_level;
    bstream_notice->write = bstreamlog_write;
    bstream_notice->read = NULL;
    bstream_notice->read_max = NULL;
    bstream_notice->peek = NULL;
    bstream_notice->peek_max = NULL;
    bstream_notice->incref = NULL;
    bstream_notice->decref = NULL;
    bstream_notice->is_available = NULL;

    bstream_info = malloc(sizeof(struct bstream) + sizeof(enum log_level));
    if(bstream_info == NULL) { free(bstream_fatal); free(bstream_error); free(bstream_notice); zlog_fini(); nlog_fatal("Could not allocate memory. (%s)", strerror(errno)); return -1; }
    enum log_level *bstream_info_level = (enum log_level *) (((void *) bstream_info) + sizeof(struct bstream));
    *bstream_info_level = LOG_LEVEL_INFO;
    bstream_info->private = bstream_info_level;
    bstream_info->write = bstreamlog_write;
    bstream_info->read = NULL;
    bstream_info->read_max = NULL;
    bstream_info->peek = NULL;
    bstream_info->peek_max = NULL;
    bstream_info->incref = NULL;
    bstream_info->decref = NULL;
    bstream_info->is_available = NULL;

    #ifdef DEBUG
        bstream_debug = malloc(sizeof(struct bstream) + sizeof(enum log_level));
        if(bstream_debug == NULL) { free(bstream_fatal); free(bstream_error); free(bstream_notice); free(bstream_info); zlog_fini(); nlog_fatal("Could not allocate memory. (%s)", strerror(errno)); return -1; }
        enum log_level *bstream_debug_level = (enum log_level *) (((void *) bstream_debug) + sizeof(struct bstream));
        *bstream_debug_level = LOG_LEVEL_DEBUG;
        bstream_debug->private = bstream_debug_level;
        bstream_debug->write = bstreamlog_write;
        bstream_debug->read = NULL;
        bstream_debug->read_max = NULL;
        bstream_debug->peek = NULL;
        bstream_debug->peek_max = NULL;
        bstream_debug->incref = NULL;
        bstream_debug->decref = NULL;
        bstream_debug->is_available = NULL;
    #else
        bstream_debug = malloc(sizeof(struct bstream));
        if(bstream_debug == NULL) { free(bstream_fatal); free(bstream_error); free(bstream_notice); free(bstream_info); zlog_fini(); nlog_fatal("Could not allocate memory. (%s)", strerror(errno)); return -1; }
        bstream_init_black_hole(bstream_debug);
    #endif

    #ifdef HAVE_FP_FROM_BSTREAM
        fp_fatal = fp_from_bstream(bstream_fatal);
        if(fp_fatal == NULL) { free(bstream_fatal); free(bstream_error); free(bstream_notice); free(bstream_info); zlog_fini(); nlog_fatal("Could not create file pointer from bstream."); return -1; }

        fp_error = fp_from_bstream(bstream_error);
        if(fp_error == NULL) { free(bstream_fatal); free(bstream_error); free(bstream_notice); free(bstream_info); zlog_fini(); nlog_fatal("Could not create file pointer from bstream."); return -1; }

        fp_warn = fp_from_bstream(bstream_warn);
        if(fp_warn == NULL) { free(bstream_fatal); free(bstream_error); free(bstream_notice); free(bstream_info); zlog_fini(); nlog_fatal("Could not create file pointer from bstream."); return -1; }

        fp_notice = fp_from_bstream(bstream_notice);
        if(fp_notice == NULL) { free(bstream_fatal); free(bstream_error); free(bstream_notice); free(bstream_info); zlog_fini(); nlog_fatal("Could not create file pointer from bstream."); return -1; }

        fp_info = fp_from_bstream(bstream_info);
        if(fp_info == NULL) { free(bstream_fatal); free(bstream_error); free(bstream_notice); free(bstream_info); zlog_fini(); nlog_fatal("Could not create file pointer from bstream."); return -1; }

        fp_debug = fp_from_bstream(bstream_debug);
        if(fp_debug == NULL) { free(bstream_fatal); free(bstream_error); free(bstream_notice); free(bstream_info); zlog_fini(); nlog_fatal("Could not create file pointer from bstream."); return -1; }
    #else
        fp_fatal    = stderr;
        fp_error    = stderr;
        fp_warn     = stderr;
        fp_notice   = stdout;
        fp_info     = stdout;
        fp_debug    = stdout;
    #endif


    #ifdef DO_REDIRECTION
        stderr = fp_error;
        stdout = fp_info;
    #endif

    #ifdef DEBUG
        nlog_info("Debug mode detected, enabling MAPI debug.");
        // TODO
    #endif

    nlog_info("Logging system was successfully initialized.");
    return 0;
}

void logging_cleanup(void) {
    nlog_debug("In logging_cleanup()");
    nlog_info("Cleaning up logging system..");
    fclose(fp_fatal);
    fclose(fp_error);
    fclose(fp_warn);
    fclose(fp_notice);
    fclose(fp_info);
    fclose(fp_debug);

    free(bstream_fatal);
    free(bstream_error);
    free(bstream_warn);
    free(bstream_notice);
    free(bstream_info);
    free(bstream_debug);

    nlog_info("Closing log..");
    zlog_fini();
}
