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

static int ini_parse_line(char *line, char *section, ini_handler handler, void *user) {
    char *start = line;
    start += strspn(start, INI_WHITESPACE);

    if (*start == '\0' || strchr(INI_COMMENT_PREFIXES, *start)) return 0;

    line[strcspn(line, "\r\n")] = '\0';

    if (*start == '[') {
        char *end = strchr(start, ']');
        if (end) {
            *end = '\0';
            strncpy(section, start + 1, INI_SECTION_STR_MAX_SIZE - 1);
            section[INI_SECTION_STR_MAX_SIZE - 1] = '\0';
        }
        return 0;
    }

    char *sep = strpbrk(start, "=:");
    if (!sep) return 0;

    *sep      = '\0';
    char *key = start;
    char *val = sep + 1;

    char *key_end = key + strlen(key) - 1;
    while (key_end > key && strchr(" \t", *key_end)) *key_end-- = '\0';

    val += strspn(val, " \t");

    if (val[0] != '\0') {
        for (size_t i = 1; val[i]; i++) {
            if ((val[i] == '#' || val[i] == ';') &&
                (val[i - 1] == ' ' || val[i - 1] == '\t')) {
                val[i] = '\0';
                break;
            }
        }
    }

    size_t val_len = strlen(val);
    while (val_len > 0 && strchr(" \t", val[val_len - 1])) {
        val[--val_len] = '\0';
    }

    return handler(section, key, val, user) ? 0 : -1;
}

bool ini_parse(const char *filename, ini_handler handler, void *user) {
    if (!handler) return false;
    FILE *file = fopen(filename, "r");
    if (!file) return false;

    char line[INI_LINE_STR_MAX_SIZE];
    char section[INI_SECTION_STR_MAX_SIZE] = "";

    while (fgets(line, sizeof(line), file) != NULL) {
        if (ini_parse_line(line, section, handler, user) != 0) {
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

        if (ini_parse_line(line, section, handler, user) != 0) return false;

        p += len;
        if (*p == '\n') p++;
    }

    return true;
}

#endif  // INI_IMPLEMENTATION
#endif  // INI_H_
