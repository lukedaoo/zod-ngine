#ifndef KEYBIND_LOAD_H
#define KEYBIND_LOAD_H

#include "keybind.h"

#ifndef KEYBIND_LOAD_LOG_ENABLED
#define KEYBIND_LOAD_LOG_ENABLED 0
#endif

#ifndef KEYBIND_SECTION_PREFIX
#define KEYBIND_SECTION_PREFIX "input."
#endif

#ifndef KEYBIND_KEY_NAME_MAX
#define KEYBIND_KEY_NAME_MAX 64
#endif

bool keybind_merge_scf(keybind_table *table, const char *path,
                       const keybind_vocab *vocab);
bool keybind_merge_ini(keybind_table *table, const char *path,
                       const keybind_vocab *vocab);
bool keybind_load_scf(keybind_table *table, const char *path, const keybind_vocab *vocab);
bool keybind_load_ini(keybind_table *table, const char *path, const keybind_vocab *vocab);

bool keybind_merge_scf_string(keybind_table *table, const char *text,
                              const keybind_vocab *vocab);
bool keybind_merge_ini_string(keybind_table *table, const char *text,
                              const keybind_vocab *vocab);

bool keybind_merge(keybind_table *table, const char *path, const keybind_vocab *vocab);
bool keybind_load(keybind_table *table, const char *path, const keybind_vocab *vocab);

#ifdef KEYBIND_LOAD_IMPLEMENTATION

#include <string.h>

#include "ini.h"
#include "log.h"
#include "scf.h"

#define KEYBIND_LOAD_WHITESPACE " \t"

typedef struct {
    keybind_table       *table;
    const keybind_vocab *vocab;
    size_t               skipped;
} keybind_parse_ctx;

static bool keybind_context_from_name(const keybind_vocab *vocab, const char *name,
                                      int *out) {
    for (size_t i = 0; i < vocab->context_count; i++) {
        if (strcmp(vocab->contexts[i].name, name) == 0) {
            *out = vocab->contexts[i].context;
            return true;
        }
    }
    return false;
}

// @info: return true to skip the key. If it returns false, it will skip the entrie file
// parser
static bool keybind_parse_handler(const char *section, const char *key, const char *value,
                                  void *user) {
    keybind_parse_ctx *ctx = (keybind_parse_ctx *)user;
    if (!section || !key || !value) return true;

    const size_t prefix_len = sizeof(KEYBIND_SECTION_PREFIX) - 1u;
    if (strncmp(section, KEYBIND_SECTION_PREFIX, prefix_len) != 0) return true;

    const char *context_name = section + prefix_len;
    if (*context_name == '\0') {
#if KEYBIND_LOAD_LOG_ENABLED
        log_warn("keybind.load: section '%s' has no context name, skipping '%s'", section,
                 key);
#endif
        ctx->skipped++;
        return true;
    }

    int context = 0;
    if (!keybind_context_from_name(ctx->vocab, context_name, &context)) {
#if KEYBIND_LOAD_LOG_ENABLED
        log_warn("keybind.load: unknown context '%s', skipping '%s'", context_name, key);
#endif
        ctx->skipped++;
        return true;
    }

    const int scancode = ctx->vocab->key_from_name(key);
    if (scancode == 0) {
#if KEYBIND_LOAD_LOG_ENABLED
        log_warn("keybind.load: unknown key '%s' in '%s', skipping", key, context_name);
#endif
        ctx->skipped++;
        return true;
    }

    const size_t trigger_len = strcspn(value, KEYBIND_LOAD_WHITESPACE);
    if (trigger_len == 0 || trigger_len >= KEYBIND_KEY_NAME_MAX) {
#if KEYBIND_LOAD_LOG_ENABLED
        log_warn("keybind.load: '%s.%s' has no usable trigger, skipping", context_name,
                 key);
#endif
        ctx->skipped++;
        return true;
    }

    char trigger_name[KEYBIND_KEY_NAME_MAX];
    memcpy(trigger_name, value, trigger_len);
    trigger_name[trigger_len] = '\0';

    int trigger = 0;
    if (!keybind_trigger_from_string(ctx->vocab, trigger_name, &trigger)) {
#if KEYBIND_LOAD_LOG_ENABLED
        log_warn("keybind.load: unknown trigger '%s' for '%s.%s', skipping", trigger_name,
                 context_name, key);
#endif
        ctx->skipped++;
        return true;
    }

    const char *rest = value + trigger_len;
    rest += strspn(rest, KEYBIND_LOAD_WHITESPACE);

    const size_t action_len = strcspn(rest, KEYBIND_LOAD_WHITESPACE);
    if (action_len == 0 || action_len >= KEYBIND_ACTION_MAX) {
#if KEYBIND_LOAD_LOG_ENABLED
        log_warn("keybind.load: '%s.%s' has no usable action name, skipping",
                 context_name, key);
#endif
        ctx->skipped++;
        return true;
    }

    char action[KEYBIND_ACTION_MAX];
    memcpy(action, rest, action_len);
    action[action_len] = '\0';

    const keybind_handle existing =
         keybind_find_by_key(ctx->table, context, scancode, trigger);
    if (existing != KEYBIND_HANDLE_INVALID) {
#if KEYBIND_LOAD_LOG_ENABLED
        keybind prev;
        if (keybind_lookup(ctx->table, existing, &prev))
            log_warn("keybind.load: '%s.%s' reassigned from '%s' to '%s'", context_name,
                     key, prev.action, action);
#endif
    }

    if (keybind_set(ctx->table, context, scancode, trigger, action) ==
        KEYBIND_HANDLE_INVALID)
        ctx->skipped++;

    return true;
}

typedef bool (*keybind_parse_func)(const char *source, void *handler, void *user);

static bool keybind_merge_internal(keybind_table *table, const char *source,
                                   const keybind_vocab *vocab,
                                   keybind_parse_func   parser) {
    if (!table || !source || !parser) return false;
    if (!keybind_vocab_valid(vocab)) {
#if KEYBIND_LOAD_LOG_ENABLED
        log_error("keybind.load: incomplete vocab");
#endif
        return false;
    }

    keybind_parse_ctx ctx = {.table = table, .vocab = vocab, .skipped = 0};
    return parser(source, keybind_parse_handler, &ctx);
}

static bool keybind_load_internal(keybind_table *table, const char *path,
                                  const keybind_vocab *vocab, keybind_parse_func parser) {
    if (!table || !path || !parser) return false;

    keybind_table scratch;
    if (!keybind_init(&scratch)) return false;

    if (!keybind_merge_internal(&scratch, path, vocab, parser)) {
        keybind_deinit(&scratch);
        return false;
    }

    keybind_deinit(table);
    *table = scratch;
    return true;
}

bool keybind_merge_scf(keybind_table *table, const char *path,
                       const keybind_vocab *vocab) {
    return keybind_merge_internal(table, path, vocab, (keybind_parse_func)scf_parse);
}

bool keybind_merge_ini(keybind_table *table, const char *path,
                       const keybind_vocab *vocab) {
    return keybind_merge_internal(table, path, vocab, (keybind_parse_func)ini_parse);
}

bool keybind_load_scf(keybind_table *table, const char *path,
                      const keybind_vocab *vocab) {
    return keybind_load_internal(table, path, vocab, (keybind_parse_func)scf_parse);
}

bool keybind_load_ini(keybind_table *table, const char *path,
                      const keybind_vocab *vocab) {
    return keybind_load_internal(table, path, vocab, (keybind_parse_func)ini_parse);
}

bool keybind_merge_scf_string(keybind_table *table, const char *text,
                              const keybind_vocab *vocab) {
    return keybind_merge_internal(table, text, vocab,
                                  (keybind_parse_func)scf_parse_string);
}

bool keybind_merge_ini_string(keybind_table *table, const char *text,
                              const keybind_vocab *vocab) {
    return keybind_merge_internal(table, text, vocab,
                                  (keybind_parse_func)ini_parse_string);
}

static keybind_parse_func keybind_parser_for_path(const char *path) {
    if (!path) return NULL;

    const char *dot = strrchr(path, '.');
    if (!dot) return NULL;

    if (strcmp(dot, ".scf") == 0) return (keybind_parse_func)scf_parse;
    if (strcmp(dot, ".ini") == 0) return (keybind_parse_func)ini_parse;
    return NULL;
}

bool keybind_merge(keybind_table *table, const char *path, const keybind_vocab *vocab) {
    keybind_parse_func parser = keybind_parser_for_path(path);
    if (!parser) {
#if KEYBIND_LOAD_LOG_ENABLED
        log_error("keybind.keybind_merge: unsupported file type '%s'", path ? path : "");
#endif
        return false;
    }
    return keybind_merge_internal(table, path, vocab, parser);
}

bool keybind_load(keybind_table *table, const char *path, const keybind_vocab *vocab) {
    keybind_parse_func parser = keybind_parser_for_path(path);
    if (!parser) {
#if KEYBIND_LOAD_LOG_ENABLED
        log_error("keybind.keybind_load: unsupported file type '%s'", path ? path : "");
#endif
        return false;
    }
    return keybind_load_internal(table, path, vocab, parser);
}

#endif
#endif
