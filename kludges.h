#ifndef KLUDGES_H
#define KLUDGES_H 1

#ifndef IN_KLUDGES_CC

#ifdef NO_SNPRINTF
#define snprintf kludge_snprintf
#endif

#endif

int kludge_snprintf(char *buf, int buflen, const char *format, ...);

#endif
