#ifndef INI_H_
#define INI_H_

#include <stdbool.h>

#include "log.h"

typedef bool (*ini_handler)(const char *section, const char *key, const char *value,
                            void *user);

bool ini_parse(const char *filename, ini_handler handler, void *user);
bool ini_parse_string(const char *string, ini_handler handler, void *user);

#ifdef INI_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

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

#ifndef INI_LOG_ENABLED
#define INI_LOG_ENABLED 0
#endif

static bool ini_parse_line(char *line, char *section, ini_handler handler, void *user) {
    char *p = line;

    while (*p == ' ' || *p == '\t') p++;

    if (*p == '\0' || strchr(INI_COMMENT_PREFIXES, *p)) return true;

    if (*p == '[') {
        char *end = p + 1;
        while (*end && *end != ']') end++;

        if (*end == ']') {
            *end = '\0';
            strncpy(section, p + 1, INI_SECTION_STR_MAX_SIZE - 1);
            section[INI_SECTION_STR_MAX_SIZE - 1] = '\0';
        }
        return true;
    }

    char *sep = p;
    while (*sep && *sep != '=' && *sep != ':') sep++;
    if (*sep == '\0') return true;

    *sep      = '\0';
    char *key = p;
    char *val = sep + 1;

    char *ke = key + strlen(key);
    while (ke > key && (ke[-1] == ' ' || ke[-1] == '\t')) *--ke = '\0';

    while (*val == ' ' || *val == '\t') val++;

    for (char *m = val; *m; m++) {
        // @Robustness: when changing the comment prefix, make sure to update this
        if ((*m == ';' || *m == '#') && (m > val && (m[-1] == ' ' || m[-1] == '\t'))) {
            *m = '\0';
            break;
        }
    }

    char *ve = val + strlen(val);
    while (ve > val &&
           (ve[-1] == ' ' || ve[-1] == '\t' || ve[-1] == '\r' || ve[-1] == '\n'))
        *--ve = '\0';

    return handler(section, key, val, user);
}

bool ini_parse(const char *filename, ini_handler handler, void *user) {
    if (!handler) return false;
    FILE *file = fopen(filename, "r");
    if (!file) {
#if INI_LOG_ENABLED
        log_warn("ini.ini_parse: could not open file '%s'", filename);
#endif
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
