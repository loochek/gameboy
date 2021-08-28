#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "log.h"

const char *log_level_str_repr[] = 
{
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR"
};

/// Logs to stdout
static void default_handler(gb_log_level_e level, const char *fmt, va_list args);

static gb_log_handler_t log_handler = default_handler;


void gb_log_set_handler(gb_log_handler_t handler)
{
    log_handler = handler;
}

void gb_log(gb_log_level_e level, const char *fmt, ...)
{
    if (log_handler == NULL)
        return;

    assert(fmt != NULL);

    va_list args = {0};
    va_start(args, fmt);
    log_handler(level, fmt, args);
    va_end(args);
}


static void default_handler(gb_log_level_e level, const char *fmt, va_list args)
{
    printf("[%s] ", log_level_str_repr[level]);
    vprintf(fmt, args);
    printf("\n");
}