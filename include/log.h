#ifndef __LOG_H__
#define __LOG_H__

#include <errno.h>
#include <assert.h>

typedef enum {
	LOG_ALL   = 0x00,
	LOG_DEBUG = 0x01,       /*调试信息*/
	LOG_INFO  = 0x02,       /*常规信息*/
	LOG_WARNING = 0x04,     /*警告信息*/
	LOG_ERROR = 0x08,       /*错误信息*/
	LOG_FATAL = 0x10,       /*致命信息*/ 
	LOG_NONE  = 0xFF
} LOG_LEVEL;

void debug_printf(LOG_LEVEL level, const char *filename, int fileline, const char *pformat, ...);

#define LOG_DEBUG(log...) \
    debug_printf(LOG_DEBUG, __FILE__, __LINE__, log)

#define LOG_INFO(log...) \
    debug_printf(LOG_INFO, __FILE__, __LINE__, log)

#define LOG_WARNING(log...) \
    debug_printf(LOG_WARNING, __FILE__, __LINE__, log)

#define LOG_ERROR(log...) \
    debug_printf(LOG_ERROR, __FILE__, __LINE__, log)

#define LOG_FATAL(log...) \
    debug_printf(LOG_FATAL, __FILE__, __LINE__, log)


void set_log_switch_level(LOG_LEVEL switch_level);

#endif //__LOG_H__
