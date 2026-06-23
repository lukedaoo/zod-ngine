#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H

typedef enum {
    FILE_NONE,     // no change since last check
    FILE_NEW,      // appeared (was absent, now exists)
    FILE_CHANGED,  // mtime or size differs from last check
    FILE_DELETED   // existed, now absent
} file_status;

typedef struct file_watcher file_watcher;

file_watcher *file_watcher_watch(const char *path);
file_status   file_watcher_check(file_watcher *w);
void          file_watcher_close(file_watcher *w);

#ifdef FILE_WATCHER_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifndef FILE_WATCHER_PATH_MAX
#define FILE_WATCHER_PATH_MAX 256
#endif

struct file_watcher {
    char   path[FILE_WATCHER_PATH_MAX];
    time_t mtime;
    bool   exists;
};

static bool file_watcher_stat(const char *path, time_t *mtime) {
    struct stat st;
    if (stat(path, &st) != 0) {
        *mtime = 0;
        return false;
    }
    *mtime = st.st_mtime;
    return true;
}

file_watcher *file_watcher_watch(const char *path) {
    size_t len = strlen(path);
    if (len >= FILE_WATCHER_PATH_MAX) {
        fprintf(stderr, "file_watcher.watch: path too long: %s\n", path);
        return NULL;
    }

    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        fprintf(stderr, "file_watcher.watch: path is a directory: %s\n", path);
        return NULL;
    }

    file_watcher *w = malloc(sizeof(file_watcher));
    if (!w) return NULL;

    memcpy(w->path, path, len + 1);
    w->exists = file_watcher_stat(path, &w->mtime);
    if (!w->exists)
        fprintf(stderr, "file_watcher.watch: path does not exist yet: %s\n", path);

    return w;
}

file_status file_watcher_check(file_watcher *w) {
    time_t mtime;
    bool   exists = file_watcher_stat(w->path, &mtime);

    file_status status = FILE_NONE;
    if (exists && !w->exists) {
        status = FILE_NEW;
    } else if (!exists && w->exists) {
        status = FILE_DELETED;
    } else if (exists && mtime != w->mtime) {
        status = FILE_CHANGED;
    }

    w->exists = exists;
    w->mtime  = mtime;

    return status;
}

void file_watcher_close(file_watcher *w) { free(w); }

#endif
#endif
