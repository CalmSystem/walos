#ifndef __SYSLOG_H
#define __SYSLOG_H

#include "stddef.h"
#include "stdarg.h"

enum syslog_severity {
    TRACE, DEBUG, INFO, WARN, ERROR, FATAL
};

/** Log context */
typedef struct syslog_ctx {
    /** Authoring entity */
    const char* group;
    /** List of sub modules. Double NULL terminated */
    const char* modules;
    enum syslog_severity severity;
} syslog_ctx;

struct syslog_handler {
    struct syslog_handler* next;
    void (*logs)(const syslog_ctx ctx, const char *content);
    void (*logf)(const syslog_ctx ctx, const char *fmt, va_list vl);
};

extern const struct syslog_handler* syslog_handlers;
/** Default screen logger (Human) */
extern const struct syslog_handler syslog_graphics_handler;
/** Default serial logger (JSON) */
extern const struct syslog_handler syslog_serial_handler;

/** Log one row */
void syslogs(const syslog_ctx ctx, const char* content);
void syslogf(const syslog_ctx ctx, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void syslogvf(const syslog_ctx ctx, const char *fmt, va_list vl) __attribute__((format(printf, 2, 0)));

#endif
