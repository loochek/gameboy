#include "gbstatus.h"

char gbstatus_str[MAX_STATUS_STR_LENGTH + 1] = {0};

const char *gbstatus_str_repr[] =
{
    "GBSTATUS_OK",
    "GBSTATUS_BAD_ALLOC",
    "GBSTATUS_IO_FAIL",
    "GBSTATUS_CPU_ILLEGAL_OP",
    "GBSTATUS_CART_FAIL",
    "GBSTATUS_SFML_FAIL",
    "GBSTATUS_NOT_IMPLEMENTED"
};