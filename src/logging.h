#ifndef STRONK_LOGGING_H
#define STRONK_LOGGING_H

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

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

#ifndef FILENAME
    #define FILENAME __FILE__
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

#endif
