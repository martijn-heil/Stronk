#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#include <sys/types.h>
#include <features.h>

#include <zlog.h>

#include "logging/logging.h"


/*
    <quote>

    In the GNU C Library, stdin, stdout, and stderr are normal variables which you can set just like any others. For example,
    to redirect the standard output to a file, you could do:
        fclose (stdout);
        stdout = fopen ("standard-output-file", "w");

    </quote>

    Source: http://www.gnu.org/software/libc/manual/html_node/Standard-Streams.html
*/

#if defined(__GNU_LIBRARY__) || defined(__GLIBC__)
    #define STANDARD_STREAMS_ASSIGNABLE 1
#else
    #define STANDARD_STREAMS_ASSIGNABLE 0
#endif

zlog_category_t *_zc;

#if STANDARD_STREAMS_ASSIGNABLE && defined(HAVE_FOPENCOOKIE)
    static ssize_t new_stdout_write(void *cookie, const char *buf, size_t size)
    {
        char *new_buf = malloc(size + 1);
        if(new_buf == NULL) return 0;
        memcpy(new_buf, buf, size);
        new_buf[size] = '\0';

        zlog(_zc, "null", sizeof("null")-1, "null", sizeof("null")-1, 0, ZLOG_LEVEL_INFO, "%s", new_buf);
        free(new_buf);
        return size;
    }

    static ssize_t new_stderr_write(void *cookie, const char *buf, size_t size)
    {
        char *new_buf = malloc(size + 1);
        if(new_buf == NULL) return 0;
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

        zlog(_zc, "null", sizeof("null")-1, "null", sizeof("null")-1, 0, ZLOG_LEVEL_ERROR, "%s", new_buf);
        free(new_buf);
        return size;
    }
#endif

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

    nlog_info("Logging system was successfully initialized.");

    nlog_info("Setting application locale to make sure we use UTF-8..");
    if(setlocale(LC_ALL, "") == NULL) // important, make sure we can use UTF-8.
    {
        nlog_warn("Could not set application locale to make sure we use UTF-8.");
    }

    #if STANDARD_STREAMS_ASSIGNABLE && defined(HAVE_FOPENCOOKIE)
        nlog_info("Redirecting stderr and stdout to log..");

        cookie_io_functions_t new_stdout_funcs;
        new_stdout_funcs.write = new_stdout_write;
        new_stdout_funcs.read = NULL;
        new_stdout_funcs.seek = NULL;
        new_stdout_funcs.close = NULL;
        FILE *new_stdout = fopencookie(NULL, "a", new_stdout_funcs);
        if(new_stdout == NULL)
        {
            nlog_error("Could not redirect stdout to log.");
            return 0;
        }
        if(setvbuf(new_stdout, NULL, _IOLBF, 0) != 0) // Ensure line buffering.
        {
            nlog_error("Could not redirect stdout to log.");
            fclose(new_stdout);
            return 0;
        }
        stdout = new_stdout;


        cookie_io_functions_t new_stderr_funcs;
        new_stderr_funcs.write = new_stderr_write;
        new_stderr_funcs.read = NULL;
        new_stderr_funcs.seek = NULL;
        new_stderr_funcs.close = NULL;
        FILE *new_stderr = fopencookie(NULL, "a", new_stderr_funcs);
        if(new_stderr == NULL)
        {
            nlog_error("Could not redirect stderr to log.");
            return 0;
        }
        if(setvbuf(new_stderr, NULL, _IOLBF, 0) != 0) // Ensure line buffering.
        {
            nlog_error("Could not redirect stderr to log.");
            fclose(new_stderr);
            return 0;
        }
        stderr = new_stderr;
    #endif

    return 0;
}

void logging_cleanup(void) {
    nlog_info("Closing log..");
    zlog_fini();
}
