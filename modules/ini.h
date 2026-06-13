#ifndef INI_H_
#define INI_H_

#include <assert.h>

typedef int (*int_handler)(const char *section,
                           const char *key,
                           const char *value,
                           void       *user);

int ini_parse(const char *filename, int_handler handler, void *user);
int ini_parse_string(const char *string, int_handler handler, void *user);

#ifndef INI_MAX_LINE
#define INI_MAX_LINE 256
#endif

#ifndef INI_MAX_SECTION
#define INI_MAX_SECTION 64
#endif

#ifdef INI_IMPLEMENTATION

static constexpr char INI_WHITESPACE[]       = " \t\r\n";
static constexpr char INI_COMMENT_PREFIXES[] = "; #";

static int ini_parse_line(char       *line,
                          char       *section,
                          int_handler handler,
                          void       *user) {
    char *start = line;
    start += strspn(start, INI_WHITESPACE);

    if (*start == '\0' || strchr(INI_COMMENT_PREFIXES, *start)) return 0;

    line[strcspn(line, "\r\n")] = '\0';

    if (*start == '[') {
        char *end = strchr(start, ']');
        if (end) {
            *end = '\0';
            strncpy(section, start + 1, INI_MAX_SECTION - 1);
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

    /* Strip a trailing "# comment" or "; comment". Must be preceded by
     * whitespace AND have at least one value char before that, so
     * "color=#ff0000" keeps its literal '#' but "123 #test" -> "123" */
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

int ini_parse(const char *filename, int_handler handler, void *user) {
    assert(handler != NULL);

    FILE *file = fopen(filename, "r");
    if (!file) return -1;

    char line[INI_MAX_LINE];
    char section[INI_MAX_SECTION] = "";

    while (fgets(line, sizeof(line), file) != NULL) {
        if (ini_parse_line(line, section, handler, user) != 0) {
            fclose(file);
            return -1;
        }
    }

    fclose(file);
    return 0;
}

int ini_parse_string(const char *string, int_handler handler, void *user) {
    assert(handler != NULL);
    assert(string != NULL);

    char line[INI_MAX_LINE];
    char section[INI_MAX_SECTION] = "";

    const char *p = string;
    while (*p) {
        size_t len      = strcspn(p, "\n");
        size_t copy_len = len < sizeof(line) - 1 ? len : sizeof(line) - 1;

        memcpy(line, p, copy_len);
        line[copy_len] = '\0';

        if (ini_parse_line(line, section, handler, user) != 0) return -1;

        p += len;
        if (*p == '\n') p++;
    }

    return 0;
}

#endif  // INI_IMPLEMENTATION
#endif  // INI_H_
