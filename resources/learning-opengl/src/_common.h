#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>

char *read_file(const char *path);

#ifdef COMMON_IMPLEMENTATION

char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "cannot open: %s\n", path); return NULL; }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    if (fseek(f, 0, SEEK_SET) != 0) {
        fprintf(stderr, "read_file: seek failed: %s\n", path);
        fclose(f);
        return NULL;
    }
    char *buf = malloc((size_t)len + 1);
    fread(buf, 1, (size_t)len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

#endif
#endif
