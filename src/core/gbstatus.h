#ifndef GBSTATUS_H
#define GBSTATUS_H

#include <stdio.h>

/**
 * Status reporting system of this project. 
 * 
 * Widely used to handle things that might go wrong in order to
 * maintain the defense programming approach in this project. 
 */

#define MAX_STATUS_STR_LENGTH 100

typedef enum
{
    GBSTATUS_OK,
    GBSTATUS_BAD_ALLOC,
    GBSTATUS_IO_FAIL,
    GBSTATUS_CPU_ILLEGAL_OP,
    GBSTATUS_INT_DISABLED,
    GBSTATUS_CART_FAIL,
    GBSTATUS_SFML_FAIL,
    GBSTATUS_NOT_IMPLEMENTED
} gbstatus_e;

/// Stores an extended info message about last non-OK status
extern char __gbstatus_str[];

extern const char *__gbstatus_str_repr[];

/**
 * Updates global status string
 */
#define GBSTATUS_STR(st_str, ...) { snprintf(__gbstatus_str, MAX_STATUS_STR_LENGTH, st_str, ##__VA_ARGS__); }

/**
 * Updates status local variable and global status string
 */
#define GBSTATUS(st, st_str, ...) { status = st; GBSTATUS_STR(st_str, ##__VA_ARGS__); }

/**
 * Peculiar "assertion" for an expression to be OK. 
 * If the value of the expression is different from GBSTATUS_OK, then this value is returned immediately.
 */
#define GBCHK(expr) { gbstatus_e __status = expr; if (__status != GBSTATUS_OK) return __status; }

#define GBSTATUS_ERR_PRINT(msg) fprintf(stderr, "[ERROR] %s. Reason: [%s] %s\n", \
                                     msg, __gbstatus_str_repr[status], __gbstatus_str)

#define GBSTATUS_WARN_PRINT(msg) fprintf(stderr, "[WARNING] %s. Reason: [%s] %s\n", \
                                         msg, __gbstatus_str_repr[status], __gbstatus_str)

#endif