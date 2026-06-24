#ifndef LOG_H
#define LOG_H

// @todo:
// - log to other streams
// - log to file
// - @thread-safe: add lock for thread safety

enum { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };
typedef struct log_event log_event;

#define log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_log(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_log(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

void log_set_level(int level);
void log_log(int level, const char *source_file, int line, const char *fmt, ...);

#ifdef LOG_IMPLEMENTATION
#include <stdarg.h>
#include <stdio.h>

static struct {
    int level;
} log_config;

void log_set_level(int level) { log_config.level = level; }

static const char *level_strings[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

#ifdef LOG_USE_SIMPLE
void log_log(int level, const char *source_file, int line, const char *fmt, ...) {
    if (level < log_config.level) {
        return;
    }

    fprintf(stderr, "%-5s %s:%d: ", level_strings[level], source_file, line);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
}
#else
#include <time.h>

#ifdef LOG_USE_COLOR
static const char *level_colors[] = {"\x1b[94m", "\x1b[36m", "\x1b[32m",
                                     "\x1b[33m", "\x1b[31m", "\x1b[35m"};
#endif

struct log_event {
    va_list     args;
    const char *fmt;
    const char *source_file;
    struct tm  *time;
    void       *dest;
    int         line;
    int         level;
};

void log_log(int level, const char *source_file, int line, const char *fmt, ...) {
    log_event ev = {
         .fmt         = fmt,
         .source_file = source_file,
         .line        = line,
         .level       = level  //
    };

    if (level < log_config.level) {
        return;
    }

    if (!ev.time) {
        time_t t = time(NULL);
        ev.time  = localtime(&t);
    }
    ev.dest = stderr;

    va_start(ev.args, fmt);
    char buf[16];
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev.time)] = '\0';
#ifdef LOG_USE_COLOR
    fprintf(ev.dest, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ", buf,
            level_colors[ev.level], level_strings[ev.level], ev.source_file, ev.line);
#else
    fprintf(ev.dest, "%s %-5s %s:%d: ", buf, level_strings[ev.level], ev.source_file,
            ev.line);
#endif
    vfprintf(ev.dest, ev.fmt, ev.args);
    fprintf(ev.dest, "\n");
    fflush(ev.dest);
    va_end(ev.args);
}
#endif  // USE_SIMPLE_LOG
#endif  // LOG_IMPLEMENTATION
#endif  // LOG_H
