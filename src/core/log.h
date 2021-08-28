#ifndef LOG_H
#define LOG_H

#include <stdarg.h>

/**
 * Logging interface
 * Logs to stdout by default
 */

typedef enum
{
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} gb_log_level_e;

extern const char *log_level_str_repr[];

typedef void (*gb_log_handler_t)(gb_log_level_e level, const char *fmt, va_list args);

/**
 * Sets log handler. Logging is disabled if NULL is passed
 * 
 * \param handler Log handler
 */
void gb_log_set_handler(gb_log_handler_t handler);

/**
 * Logging function
 * 
 * \param level Log level
 * \param fmt Format string
 */
void gb_log(gb_log_level_e level, const char *fmt, ...);

#endif