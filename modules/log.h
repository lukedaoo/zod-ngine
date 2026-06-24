#ifndef LOG_H
#define LOG_H

#include <stdio.h>

// @todo:
// - @thread-safe: add lock for thread safety

enum { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };

#define log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_log(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_log(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

void log_set_level(int level);
void log_set_fp(FILE *fp, int level);
void log_log(int level, const char *source_file, int line, const char *fmt, ...);

#ifdef LOG_IMPLEMENTATION
#include <stdarg.h>
#include <time.h>

static struct {
    int level;

    FILE *fp;
    int   fp_level;
} log_config;

void log_set_level(int level) { log_config.level = level; }

void log_set_fp(FILE *fp, int level) {
    log_config.fp       = fp;
    log_config.fp_level = level;
}

static const char *level_strings[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

#ifdef LOG_USE_COLOR
static const char *level_colors[] = {"\x1b[94m", "\x1b[36m", "\x1b[32m",
                                     "\x1b[33m", "\x1b[31m", "\x1b[35m"};
#endif

static void log_emit(FILE *fp, const char *fmt, va_list ap) {
    vfprintf(fp, fmt, ap);
    fputc('\n', fp);
    fflush(fp);
}

void log_log(int level, const char *source_file, int line, const char *fmt, ...) {
    va_list args;

#ifndef LOG_USE_SIMPLE
    char       time_buf[20];
    time_t     now = time(NULL);
    struct tm *lt  = localtime(&now);
#endif

    if (level >= log_config.level) {
#if defined(LOG_USE_SIMPLE) && defined(LOG_USE_COLOR)
        fprintf(stderr, "%s%-5s\x1b[0m %s:%d: ", level_colors[level],
                level_strings[level], source_file, line);
#elif defined(LOG_USE_SIMPLE)
        fprintf(stderr, "[%-5s] %s:%d: ", level_strings[level], source_file, line);
#elif defined(LOG_USE_COLOR)
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", lt);
        fprintf(stderr, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ", time_buf,
                level_colors[level], level_strings[level], source_file, line);
#else
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", lt);
        fprintf(stderr, "%s %-5s %s:%d: ", time_buf, level_strings[level], source_file,
                line);
#endif
        va_start(args, fmt);
        log_emit(stderr, fmt, args);
        va_end(args);
    }

    if (log_config.fp && level >= log_config.fp_level) {
#ifdef LOG_USE_SIMPLE
        fprintf(log_config.fp, "%-5s %s:%d: ", level_strings[level], source_file, line);
#else
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", lt);
        fprintf(log_config.fp, "%s %-5s %s:%d: ", time_buf, level_strings[level],
                source_file, line);
#endif
        va_start(args, fmt);
        log_emit(log_config.fp, fmt, args);
        va_end(args);
    }
}
#endif  // LOG_IMPLEMENTATION
#endif  // LOG_H
