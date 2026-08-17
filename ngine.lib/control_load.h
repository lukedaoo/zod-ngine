#ifndef CONTROL_LOAD_H
#define CONTROL_LOAD_H

#include "control.h"

#ifndef CONTROL_LOAD_LOG_ENABLED
#define CONTROL_LOAD_LOG_ENABLED 0
#endif

#ifndef CONTROL_SECTION_PREFIX
#define CONTROL_SECTION_PREFIX "input."
#endif

#ifndef CONTROL_KEY_NAME_MAX
#define CONTROL_KEY_NAME_MAX 64
#endif

bool control_merge_scf(control_table *table, const char *path,
                       const control_vocab *vocab);
bool control_merge_ini(control_table *table, const char *path,
                       const control_vocab *vocab);
bool control_load_scf(control_table *table, const char *path, const control_vocab *vocab);
bool control_load_ini(control_table *table, const char *path, const control_vocab *vocab);

bool control_merge_scf_string(control_table *table, const char *text,
                              const control_vocab *vocab);
bool control_merge_ini_string(control_table *table, const char *text,
                              const control_vocab *vocab);

bool control_merge(control_table *table, const char *path, const control_vocab *vocab);
bool control_load(control_table *table, const char *path, const control_vocab *vocab);

#ifdef CONTROL_LOAD_IMPLEMENTATION

#include <string.h>

#include "ini.h"
#include "log.h"
#include "scf.h"

#define CONTROL_LOAD_WHITESPACE " \t"

typedef struct {
    control_table       *table;
    const control_vocab *vocab;
    size_t               skipped;
} control_parse_ctx;

static bool control_context_from_name(const control_vocab *vocab, const char *name,
                                      action_mode *out) {
    for (size_t i = 0; i < vocab->context_count; i++) {
        if (strcmp(vocab->contexts[i].name, name) == 0) {
            *out = vocab->contexts[i].context;
            return true;
        }
    }
    return false;
}

static bool control_parse_handler(const char *section, const char *key, const char *value,
                                  void *user) {
    control_parse_ctx *ctx = (control_parse_ctx *)user;
    if (!section || !key || !value) return true;

    const size_t prefix_len = sizeof(CONTROL_SECTION_PREFIX) - 1u;
    if (strncmp(section, CONTROL_SECTION_PREFIX, prefix_len) != 0) return true;

    const char *context_name = section + prefix_len;
    if (*context_name == '\0') {
#if CONTROL_LOAD_LOG_ENABLED
        log_warn("control.load: section '%s' has no context name, skipping '%s'", section,
                 key);
#endif
        ctx->skipped++;
        return true;
    }

    action_mode context = 0;
    if (!control_context_from_name(ctx->vocab, context_name, &context)) {
#if CONTROL_LOAD_LOG_ENABLED
        log_warn("control.load: unknown context '%s', skipping '%s'", context_name, key);
#endif
        ctx->skipped++;
        return true;
    }

    const size_t key_len = strcspn(value, CONTROL_LOAD_WHITESPACE);
    if (key_len == 0 || key_len >= CONTROL_KEY_NAME_MAX) {
#if CONTROL_LOAD_LOG_ENABLED
        log_warn("control.load: '%s.%s' has no usable key name, skipping", context_name,
                 key);
#endif
        ctx->skipped++;
        return true;
    }

    char key_name[CONTROL_KEY_NAME_MAX];
    memcpy(key_name, value, key_len);
    key_name[key_len] = '\0';

    const int scancode = ctx->vocab->key_from_name(key_name);
    if (scancode == 0) {
#if CONTROL_LOAD_LOG_ENABLED
        log_warn("control.load: unknown key '%s' for '%s.%s', skipping", key_name,
                 context_name, key);
#endif
        ctx->skipped++;
        return true;
    }

    const char *rest = value + key_len;
    rest += strspn(rest, CONTROL_LOAD_WHITESPACE);

    size_t                      trigger_count = 0;
    const control_trigger_name *triggers =
         control_trigger_table(ctx->vocab, &trigger_count);
    action_trigger_type trigger = triggers[0].trigger;

    if (*rest != '\0') {
        const size_t trigger_len = strcspn(rest, CONTROL_LOAD_WHITESPACE);
        char         trigger_name[CONTROL_KEY_NAME_MAX] = "";
        bool         resolved                           = false;

        if (trigger_len < CONTROL_KEY_NAME_MAX) {
            memcpy(trigger_name, rest, trigger_len);
            trigger_name[trigger_len] = '\0';
            resolved = control_trigger_from_string(ctx->vocab, trigger_name, &trigger);
        }

        if (!resolved) {
#if CONTROL_LOAD_LOG_ENABLED
            log_warn("control.load: unknown trigger '%s' for '%s.%s', skipping",
                     trigger_name, context_name, key);
#endif
            ctx->skipped++;
            return true;
        }
    }

    if (control_set(ctx->table, context, key, scancode, trigger) ==
        CONTROL_HANDLE_INVALID)
        ctx->skipped++;

    return true;
}

typedef bool (*control_parse_func)(const char *source, void *handler, void *user);

static bool control_merge_internal(control_table *table, const char *source,
                                   const control_vocab *vocab,
                                   control_parse_func   parser) {
    if (!table || !source || !parser) return false;
    if (!control_vocab_valid(vocab)) {
#if CONTROL_LOAD_LOG_ENABLED
        log_error("control.load: incomplete vocab");
#endif
        return false;
    }

    control_parse_ctx ctx = {.table = table, .vocab = vocab, .skipped = 0};
    return parser(source, control_parse_handler, &ctx);
}

static bool control_load_internal(control_table *table, const char *path,
                                  const control_vocab *vocab, control_parse_func parser) {
    if (!table || !path || !parser) return false;

    control_table scratch;
    if (!control_init(&scratch)) return false;

    if (!control_merge_internal(&scratch, path, vocab, parser)) {
        control_deinit(&scratch);
        return false;
    }

    control_deinit(table);
    *table = scratch;
    return true;
}

bool control_merge_scf(control_table *table, const char *path,
                       const control_vocab *vocab) {
    return control_merge_internal(table, path, vocab, (control_parse_func)scf_parse);
}

bool control_merge_ini(control_table *table, const char *path,
                       const control_vocab *vocab) {
    return control_merge_internal(table, path, vocab, (control_parse_func)ini_parse);
}

bool control_load_scf(control_table *table, const char *path,
                      const control_vocab *vocab) {
    return control_load_internal(table, path, vocab, (control_parse_func)scf_parse);
}

bool control_load_ini(control_table *table, const char *path,
                      const control_vocab *vocab) {
    return control_load_internal(table, path, vocab, (control_parse_func)ini_parse);
}

bool control_merge_scf_string(control_table *table, const char *text,
                              const control_vocab *vocab) {
    return control_merge_internal(table, text, vocab,
                                  (control_parse_func)scf_parse_string);
}

bool control_merge_ini_string(control_table *table, const char *text,
                              const control_vocab *vocab) {
    return control_merge_internal(table, text, vocab,
                                  (control_parse_func)ini_parse_string);
}

static control_parse_func control_parser_for_path(const char *path) {
    if (!path) return NULL;

    const char *dot = strrchr(path, '.');
    if (!dot) return NULL;

    if (strcmp(dot, ".scf") == 0) return (control_parse_func)scf_parse;
    if (strcmp(dot, ".ini") == 0) return (control_parse_func)ini_parse;
    return NULL;
}

bool control_merge(control_table *table, const char *path, const control_vocab *vocab) {
    control_parse_func parser = control_parser_for_path(path);
    if (!parser) {
#if CONTROL_LOAD_LOG_ENABLED
        log_error("control.control_merge: unsupported file type '%s'", path ? path : "");
#endif
        return false;
    }
    return control_merge_internal(table, path, vocab, parser);
}

bool control_load(control_table *table, const char *path, const control_vocab *vocab) {
    control_parse_func parser = control_parser_for_path(path);
    if (!parser) {
#if CONTROL_LOAD_LOG_ENABLED
        log_error("control.control_load: unsupported file type '%s'", path ? path : "");
#endif
        return false;
    }
    return control_load_internal(table, path, vocab, parser);
}

#endif
#endif
