#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define IN_KLUDGES_CC 1
#include "kludges.h"

// NOTE: this is not thread-safe, but since FW does not use threads yet,
// it's OK for now.

static char bigbuf[100000];

int kludge_snprintf(char *buf, int buflen, const char *format, ...) {
    int n;
    va_list ap;
    va_start(ap, format);
    n = vsprintf(bigbuf, format, ap);
    va_end(ap);
    strncpy(buf, bigbuf, buflen);
    buf[buflen - 1] = 0;
    return n;
}
