#ifndef __LOG_H__
#define __LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

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

#define log_debug(log...) \
    debug_printf(LOG_DEBUG, __FILE__, __LINE__, log)

#define log_info(log...) \
    debug_printf(LOG_INFO, __FILE__, __LINE__, log)

#define log_warning(log...) \
    debug_printf(LOG_WARNING, __FILE__, __LINE__, log)

#define log_error(log...) \
    debug_printf(LOG_ERROR, __FILE__, __LINE__, log)

#define log_fatal(log...) \
    debug_printf(LOG_FATAL, __FILE__, __LINE__, log)


void set_log_switch_level(LOG_LEVEL switch_level);

#ifdef __cplusplus
}
#endif

#endif //__LOG_H__

