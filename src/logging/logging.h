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

#ifndef STRONK_LOGGING_H
#define STRONK_LOGGING_H

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include <ninio/bstream.h>

#include <zlog.h>

int logging_init(void);
void logging_cleanup(void);


extern zlog_category_t *_zc; // don't access this..

#undef zlog_fatal
#undef zlog_error
#undef zlog_warn
#undef zlog_notice
#undef zlog_info
#undef zlog_debug

#ifndef __FILENAME__
    #if defined(WIN32) || defined(_WIN32) || defined(_WIN64) && !defined(__CYGWIN__)
        #define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
    #else
        #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
    #endif
#endif

#define zlog_fatal(cat, ...) \
	zlog(cat, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	ZLOG_LEVEL_FATAL, __VA_ARGS__)
#define zlog_error(cat, ...) \
	zlog(cat, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	ZLOG_LEVEL_ERROR, __VA_ARGS__)
#define zlog_warn(cat, ...) \
	zlog(cat, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	ZLOG_LEVEL_WARN, __VA_ARGS__)
#define zlog_notice(cat, ...) \
	zlog(cat, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	ZLOG_LEVEL_NOTICE, __VA_ARGS__)
#define zlog_info(cat, ...) \
	zlog(cat, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	ZLOG_LEVEL_INFO, __VA_ARGS__)
#define zlog_debug(cat, ...) \
	zlog(cat, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	ZLOG_LEVEL_DEBUG, __VA_ARGS__)

// nlog for ninlog ofcourse :P
// on a serious note, this is used to make migrating to other logging systems easier.
#define nlog_fatal(...)    zlog_fatal(_zc, __VA_ARGS__)
#define nlog_error(...)    zlog_error(_zc, __VA_ARGS__)
#define nlog_warn(...)     zlog_warn(_zc, __VA_ARGS__)
#define nlog_notice(...)   zlog_notice(_zc, __VA_ARGS__)
#define nlog_info(...)     zlog_info(_zc, __VA_ARGS__)
#define nlog_debug(...)    zlog_debug(_zc, __VA_ARGS__)

extern struct bstream *bstream_fatal;
extern struct bstream *bstream_error;
extern struct bstream *bstream_warn;
extern struct bstream *bstream_notice;
extern struct bstream *bstream_info;
extern struct bstream *bstream_debug;

// these function pointer variants are sub-optimal, only use them if you really have no other choice.
extern FILE *fp_fatal;
extern FILE *fp_error;
extern FILE *fp_warn;
extern FILE *fp_notice;
extern FILE *fp_info;
extern FILE *fp_debug;

#endif
