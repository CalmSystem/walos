#include "syslog.h"
#include "doprnt.h"
#include "graphics.h"
#include "string.h"
#include "asm.h"

//FIXME: must be non blocking thread safe...

enum fmt { HUMAN, JSON };
typedef void (*outbytes)(const char*, int);

static const struct {
    const char* str;
    int len;
} svs[] = {
    {"TRACE", 5},
    {"DEBUG", 5},
    {"INFO", 4},
    {"WARN", 4},
    {"ERROR", 5},
    {"FATAL", 5},
};

static inline void json_escape(const char* t, outbytes out) {
    //TODO: must escape json
    out(t, strlen(t));
}
static inline void log_start(syslog_ctx ctx, enum fmt fmt, outbytes out) {
    switch (fmt)
    {
    case HUMAN:
        out(ctx.group, strlen(ctx.group));
        if (ctx.modules) {
            const char *p = ctx.modules;
            while (*p != '\0')
            {
                out(" ", 1);
                out(p, strlen(p));
                p += (strlen(p) + 1);
            }
        }
        out(" (", 2);
        out(svs[ctx.severity].str, svs[ctx.severity].len);
        out("): ", 3);
        break;
    
    case JSON:
        out("{\"m\"=\"", 11);
        json_escape(ctx.group, out);

        out("\",\"ms\"=[", 8);
        if (ctx.modules) {
            const char *p = ctx.modules;
            while (*p != '\0') {
                out("\"", 1);
                json_escape(p, out);
                out("\"", 1);
                p += (strlen(p) + 1);
                if (*p != '\0') out(",", 1);
            }
        }
        out("],\"s\"=\"", 7);
        out(svs[ctx.severity].str, svs[ctx.severity].len);
        out("\",\"c\"=\"", 7);
        break;
    }
}
static inline void log_end(int breaked, enum fmt fmt, outbytes out) {
    switch (fmt)
    {
    case HUMAN:
        if (!breaked) out("\n", 1);
        break;

    case JSON:
        out("\"}\n", 3);
        break;
    }
}

static void graphics_putc(char* arg, int c) {
    (void)arg;
    char t = c;
    putbytes(&t, 1);
}
static void graphics_human_logs(syslog_ctx ctx, const char* content) {
    log_start(ctx, HUMAN, putbytes);
    int len = strlen(content);
    putbytes(content, len);
    log_end(*(content + len - 1) == '\n', HUMAN, putbytes);
}
static void graphics_human_logf(syslog_ctx ctx, const char* fmt, va_list vl) {
    log_start(ctx, HUMAN, putbytes);
    _doprnt(fmt, vl, 0, (void (*)())graphics_putc, NULL);
    log_end(*(fmt + strlen(fmt) - 1) == '\n', HUMAN, putbytes);
}

#define COM1 0x3F8
static void serial_putc(char* arg, int c) {
    if (c == '\n') serial_putc(arg, '\r');
    while ((io_read8(COM1 + 5) & 0x20) == 0);
    io_write8(COM1, c);
}
static void serial_putbytes(const char* s, int len) {
    const char *c = s;
    for (int i = 0; i < len && *c != '\0'; i++)
        serial_putc(NULL, *(c++));
}
static void serial_json_logs(syslog_ctx ctx, const char* content) {
    log_start(ctx, JSON, serial_putbytes);
    int len = strlen(content);
    serial_putbytes(content, len);
    log_end(*(content + len - 1) == '\n', JSON, serial_putbytes);
}
static void serial_json_logf(syslog_ctx ctx, const char* fmt, va_list vl) {
    log_start(ctx, JSON, serial_putbytes);
    _doprnt(fmt, vl, 0, (void (*)())serial_putc, NULL);
    log_end(*(fmt + strlen(fmt) - 1) == '\n', JSON, serial_putbytes);
}

const struct syslog_handler* syslog_handlers = NULL;
const struct syslog_handler syslog_graphics_handler = {NULL, graphics_human_logs, graphics_human_logf};
const struct syslog_handler syslog_serial_handler = {NULL, serial_json_logs, serial_json_logf};

void syslogf(const syslog_ctx ctx, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    syslogvf(ctx, fmt, args);
    va_end(args);

    return;
}
void syslogs(const syslog_ctx ctx, const char *content) {
    for (const struct syslog_handler *h = syslog_handlers; h != NULL; h = h->next)
        h->logs(ctx, content);
}
void syslogvf(const syslog_ctx ctx, const char *fmt, va_list vl) {
    for (const struct syslog_handler *h = syslog_handlers; h != NULL; h = h->next)
        h->logf(ctx, fmt, vl);
}
