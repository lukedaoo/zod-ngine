/*
 * @brief Specification for the custom application configuration format.
 *
 * --- Syntax Overview ---
 * * 1. Comments:
 * Any text following a ';' character is ignored, whether at the start
 * of a line or following a configuration value.
 *
 * 2. Sections:
 * Defined by the ':/' prefix. All subsequent key-value pairs belong
 * to the most recently defined section.
 *
 * 3. Key-Value Pairs:
 * Consist of a key and a value separated by one or more whitespace
 * characters (spaces or tabs).
 *
 * --- Example ---
 * * ; Initialize graphics settings
 * :/display
 * vsync true ; Enable vertical synchronization
 *
 * ; Initialize audio levels
 * :/audio
 * master_volumn 1.0F
 */
#ifndef SCF_H
#define SCF_H

typedef bool (*scf_handler)(const char *section,
                            const char *key,
                            const char *value,
                            void       *user);

bool scf_parse(const char *filename, scf_handler handler, void *user);
bool scf_parse_string(const char *string, scf_handler handler, void *user);

#ifdef SCF_IMPLEMENTATION
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifndef SCF_LINE_STR_MAX_SIZE
#define SCF_LINE_STR_MAX_SIZE 1024
#endif

#ifndef SCF_SECTION_STR_MAX_SIZE
#define SCF_SECTION_STR_MAX_SIZE 128
#endif

#ifndef SCF_WHITESPACE
#define SCF_WHITESPACE " \t\r\n"
#endif

#ifndef SCF_COMMENT_PREFIXES
#define SCF_COMMENT_PREFIXES ";"
#endif

static bool scf_process_line(char *line, char *section, scf_handler handler, void *user) {
    char *start = line + strspn(line, " \t");
    if (*start == '\0' || strchr(SCF_COMMENT_PREFIXES, *start)) return true;

    if (strncmp(start, ":/", 2) == 0) {
        char  *sec_name = start + 2;
        char  *end      = strpbrk(sec_name, " \t\r\n;");
        size_t len      = end ? (size_t)(end - sec_name) : strlen(sec_name);
        if (len >= SCF_SECTION_STR_MAX_SIZE) len = SCF_SECTION_STR_MAX_SIZE - 1;
        strncpy(section, sec_name, len);
        section[len] = '\0';
        return true;
    }

    char *key = strtok(start, SCF_WHITESPACE);
    char *val = strtok(NULL, "");

    if (key && val) {
        val += strspn(val, " \t");

        char *comment = strpbrk(val, SCF_COMMENT_PREFIXES);
        if (comment) *comment = '\0';

        size_t vlen = strlen(val);
        while (vlen > 0 && isspace((unsigned char)val[vlen - 1])) val[--vlen] = '\0';

        if (vlen > 0) return handler(section, key, val, user);
    }
    return true;
}

bool scf_parse(const char *filename, scf_handler handler, void *user) {
    if (!handler) return false;
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "scf.scf_parse: could not open file '%s'\n", filename);
        return false;
    }

    char line[SCF_LINE_STR_MAX_SIZE];
    char section[SCF_SECTION_STR_MAX_SIZE] = "";

    while (fgets(line, sizeof(line), file)) {
        if (!scf_process_line(line, section, handler, user)) {
            return false;
        }
    }
    fclose(file);
    return true;
}

bool scf_parse_string(const char *string, scf_handler handler, void *user) {
    if (!handler || !string) return false;
    char buffer[SCF_LINE_STR_MAX_SIZE];
    char section[SCF_SECTION_STR_MAX_SIZE] = "";

    const char *p = string;
    while (*p) {
        size_t len      = strcspn(p, "\n");
        size_t copy_len = len < sizeof(buffer) - 1 ? len : sizeof(buffer) - 1;
        memcpy(buffer, p, copy_len);
        buffer[copy_len] = '\0';

        if (!scf_process_line(buffer, section, handler, user)) return false;

        p += len;
        if (*p == '\n') p++;
    }
    return true;
}

#endif
#endif
