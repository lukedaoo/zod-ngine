#ifndef INI_H_
#define INI_H_

typedef bool (*ini_handler)(const char *section,
                            const char *key,
                            const char *value,
                            void       *user);

bool ini_parse(const char *filename, ini_handler handler, void *user);
bool ini_parse_string(const char *string, ini_handler handler, void *user);

#ifdef INI_IMPLEMENTATION

#include <stdio.h>
#include <string.h>

#ifndef INI_LINE_STR_MAX_SIZE
#define INI_LINE_STR_MAX_SIZE 1024
#endif

#ifndef INI_SECTION_STR_MAX_SIZE
#define INI_SECTION_STR_MAX_SIZE 128
#endif

#ifndef INI_WHITESPACE
#define INI_WHITESPACE " \t\r\n"
#endif

#ifndef INI_COMMENT_PREFIXES
#define INI_COMMENT_PREFIXES "; #"
#endif

// @perf: no strpbrk
static bool ini_parse_line(char *line, char *section, ini_handler handler, void *user) {
    char *start = line;
    start += strspn(start, INI_WHITESPACE);

    if (*start == '\0' || strchr(INI_COMMENT_PREFIXES, *start)) return true;

    if (*start == '[') {
        char *end = strchr(start, ']');
        if (end) {
            *end = '\0';
            strncpy(section, start + 1, INI_SECTION_STR_MAX_SIZE - 1);
            section[INI_SECTION_STR_MAX_SIZE - 1] = '\0';
        }
        return true;
    }

    char *sep = strpbrk(start, "=:");
    if (!sep) return true;

    *sep      = '\0';
    char *key = start;
    char *val = sep + 1;

    char *key_end = key + strlen(key) - 1;
    while (key_end > key && strchr(" \t", *key_end)) *key_end-- = '\0';

    val += strspn(val, " \t");

    if (val[0] != '\0') {
        char *marker = val + 1;
        while ((marker = strpbrk(marker, ";#")) != NULL) {
            if (marker[-1] == ' ' || marker[-1] == '\t') {
                *marker = '\0';
                break;
            }
            marker++;
        }
    }

    size_t val_len = strlen(val);
    while (val_len > 0 && strchr(" \t\r\n", val[val_len - 1])) {
        val[--val_len] = '\0';
    }

    return handler(section, key, val, user);
}

bool ini_parse(const char *filename, ini_handler handler, void *user) {
    if (!handler) return false;
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "ini.ini_parse: could not open file '%s'\n", filename);
        return false;
    }

    char line[INI_LINE_STR_MAX_SIZE];
    char section[INI_SECTION_STR_MAX_SIZE] = "";

    while (fgets(line, sizeof(line), file) != NULL) {
        if (!ini_parse_line(line, section, handler, user)) {
            fclose(file);
            return false;
        }
    }

    fclose(file);
    return true;
}

bool ini_parse_string(const char *string, ini_handler handler, void *user) {
    if (!handler) return false;
    if (!string) return false;

    char line[INI_LINE_STR_MAX_SIZE];
    char section[INI_SECTION_STR_MAX_SIZE] = "";

    const char *p = string;
    while (*p) {
        size_t len      = strcspn(p, "\n");
        size_t copy_len = len < sizeof(line) - 1 ? len : sizeof(line) - 1;

        memcpy(line, p, copy_len);
        line[copy_len] = '\0';

        if (!ini_parse_line(line, section, handler, user)) return false;

        p += len;
        if (*p == '\n') p++;
    }

    return true;
}

#endif  // INI_IMPLEMENTATION
#endif  // INI_H_
