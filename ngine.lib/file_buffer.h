#ifndef FILE_BUFFER_H
#define FILE_BUFFER_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    const uint8_t *data;
    size_t         size;
} file_buffer;

const file_buffer *read_file_as_string(const char *path);
const file_buffer *read_file_as_binary(const char *path);
void               free_file_buffer(const file_buffer *fb);

#ifdef IO_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

#ifndef FILE_BUFFER_LOG_ENABLED
#define FILE_BUFFER_LOG_ENABLED 0
#endif

static uint8_t *fb_load_file(const char *path, size_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
#if FILE_BUFFER_LOG_ENABLED
        log_error("file_buffer: cannot open: %s", path);
#endif
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
#if FILE_BUFFER_LOG_ENABLED
        log_error("file_buffer: seek end failed: %s", path);
#endif
        fclose(f);
        return NULL;
    }

    long len = ftell(f);
    if (len < 0) {
#if FILE_BUFFER_LOG_ENABLED
        log_error("file_buffer: ftell failed: %s", path);
#endif
        fclose(f);
        return NULL;
    }

    if (fseek(f, 0, SEEK_SET) != 0) {
#if FILE_BUFFER_LOG_ENABLED
        log_error("file_buffer: fseek to start failed: %s", path);
#endif
        fclose(f);
        return NULL;
    }

    uint8_t *buf = malloc((size_t)len);
    if (!buf) {
#if FILE_BUFFER_LOG_ENABLED
        log_error("file_buffer: malloc failed for %ld bytes: %s", len, path);
#endif
        fclose(f);
        return NULL;
    }

    size_t n = fread(buf, 1, (size_t)len, f);
    fclose(f);

    if (n != (size_t)len) {
#if FILE_BUFFER_LOG_ENABLED
        log_error("file_buffer: short read: expected %ld, got %zu: %s", len, n, path);
#endif
        free(buf);
        return NULL;
    }

    if (out_size) *out_size = (size_t)len;
    return buf;
}

const file_buffer *read_file_as_string(const char *path) {
    size_t   len = 0;
    uint8_t *raw = fb_load_file(path, &len);
    if (!raw) return NULL;

    char *str = malloc(len + 1);
    if (!str) {
#if FILE_BUFFER_LOG_ENABLED
        log_error("file_buffer: malloc failed for %zu bytes: %s", len + 1, path);
#endif
        free(raw);
        return NULL;
    }

    memcpy(str, raw, len);
    str[len] = '\0';
    free(raw);

    file_buffer *fb = malloc(sizeof(file_buffer));
    fb->data        = (const uint8_t *)str;
    fb->size        = len;
    return fb;
}

const file_buffer *read_file_as_binary(const char *path) {
    size_t   len = 0;
    uint8_t *raw = fb_load_file(path, &len);
    if (!raw) return NULL;

    file_buffer *fb = malloc(sizeof(file_buffer));
    fb->data        = raw;
    fb->size        = len;
    return fb;
}

void free_file_buffer(const file_buffer *fb) {
    if (!fb) return;
    free((void *)fb->data);
    free((void *)fb);
}

#endif
#endif
