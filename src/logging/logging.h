/*
    MIT License

    Copyright (c) 2016-2018 Martijn Heil

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
#include <ninio/logging.h>

#include <zlog.h>

int logging_init(void);
void logging_cleanup(void);

extern struct logger *nlogger;


// nlog for ninlog ofcourse :P
// on a serious note, this is used to make migrating to other logging systems easier.

#define nlog_fatal(...)    log_fatal(nlogger, __VA_ARGS__)
#define nlog_error(...)    log_error(nlogger, __VA_ARGS__)
#define nlog_warn(...)     log_warn(nlogger, __VA_ARGS__)
#define nlog_notice(...)   log_notice(nlogger, __VA_ARGS__)
#define nlog_info(...)     log_info(nlogger, __VA_ARGS__)
#define nlog_debug(...)    log_debug(nlogger, __VA_ARGS__)

extern struct bstream *bstream_fatal;
extern struct bstream *bstream_error;
extern struct bstream *bstream_warn;
extern struct bstream *bstream_notice;
extern struct bstream *bstream_info;
extern struct bstream *bstream_debug;

// these file pointer variants are sub-optimal, only use them if you really have no other choice.
extern FILE *fp_fatal;
extern FILE *fp_error;
extern FILE *fp_warn;
extern FILE *fp_notice;
extern FILE *fp_info;
extern FILE *fp_debug;

#endif
