#ifndef CARG_TO_CVAR_H
#define CARG_TO_CVAR_H

#include "carg.h"
#include "cvar.h"
#include "log.h"

bool carg_entry_to_cvars(const carg *carg, const char **names, const size_t names_count,
                         cvar_table *table);

bool carg_table_to_cvars(const carg_table *cargs, const char ***names_per_carg,
                         const size_t *names_count_per_carg, cvar_table *table);

#if defined(CVAR_IMPLEMENTATION) && defined(CARG_IMPLEMENTATION)

#ifndef CARG_TO_CVAR_LOG_ENABLED
#define CARG_TO_CVAR_LOG_ENABLED 0
#endif

bool carg_entry_to_cvars(const carg *carg, const char **names, const size_t names_count,
                         cvar_table *table) {
    if (!carg || !names || !table) {
        return false;
    }

    if (!carg->present) {
#if CARG_TO_CVAR_LOG_ENABLED
        log_debug(
             "cvar.carg_entry_to_cvars: flag '%s' not present, skipping (returning "
             "true)",
             carg->flag);
#endif
        return true;
    }

    if (carg->type == CARG_BOOL) {
        if (names_count != 1) {
#if CARG_TO_CVAR_LOG_ENABLED
            log_error("carg.carg_entry_to_cvars: Expected 1 name for bool flag");
#endif
            return false;
        }
        return cvar_set_bool(table, names[0], carg->value.b);
    }

    if (names_count != carg->count) {
#if CARG_TO_CVAR_LOG_ENABLED
        log_error("carg.carg_entry_to_cvars: Expected %zu names for flag", carg->count);
#endif
        return false;
    }

    for (size_t i = 0; i < carg->count; i++) {
        bool ok = false;
        if (carg->type == CARG_INT) {
            ok = cvar_set_int(table, names[i], carg->value.i[i]);
        } else if (carg->type == CARG_FLOAT) {
            ok = cvar_set_float(table, names[i], carg->value.f[i]);
        } else if (carg->type == CARG_STRING) {
            ok = cvar_set_string(table, names[i], carg->value.s[i]);
        }

        if (!ok) return false;
#if CARG_TO_CVAR_LOG_ENABLED
        if (ok) {
            log_debug("carg.carg_entry_to_cvars: registered %s from %s for %zu", names[i],
                      carg->flag, i);
        }
#endif
    }

    return true;
}

bool carg_table_to_cvars(const carg_table *cargs, const char ***names_per_carg,
                         const size_t *names_count_per_carg, cvar_table *table) {
    if (!cargs || !names_per_carg || !names_count_per_carg || !table) {
        return false;
    }
    bool ok = false;
    for (size_t i = 0; i < cargs->size; i++) {
        if (carg_entry_to_cvars(&cargs->data[i], names_per_carg[i],
                                names_count_per_carg[i], table)) {
            ok = true;
        }
    }

    return ok;
}
#endif

#endif
