#ifndef CARG_TO_CVAR_H
#define CARG_TO_CVAR_H

#include "carg.h"
#include "cvar.h"

bool carg_entry_to_cvars(const carg_t *carg,
                         const char  **names,
                         const size_t  names_count,
                         cvar_table   *table);

bool carg_table_to_cvars(const carg_table *cargs,
                         const char     ***names_per_carg,
                         const size_t     *names_count_per_carg,
                         cvar_table       *table);

#if defined(CVAR_IMPLEMENTATION) && defined(CARG_IMPLEMENTATION)
bool carg_entry_to_cvars(const carg_t *carg,
                         const char  **names,
                         const size_t  names_count,
                         cvar_table   *table) {
    if (!carg || !names || !table) {
        return false;
    }

    if (carg->type == CARG_BOOL) {
        if (names_count != 1) {
#ifdef MODULE_LOG_ENABLED
            fprintf(stderr, "carg.carg_entry_to_cvars: Expected 1 name for bool flag\n");
#endif
            return false;
        }
        return cvar_set_bool(table, names[0], carg->value.b);
    }

    if (names_count != carg->count) {
#ifdef MODULE_LOG_ENABLED
        fprintf(stderr, "carg.carg_entry_to_cvars: Expected %zu names for flag\n",
                carg->count);
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
    }

    return true;
}

bool carg_table_to_cvars(const carg_table *cargs,
                         const char     ***names_per_carg,
                         const size_t     *names_count_per_carg,
                         cvar_table       *table) {
    if (!cargs || !names_per_carg || !names_count_per_carg || !table) {
        return false;
    }

    for (size_t i = 0; i < cargs->size; i++) {
        if (!carg_entry_to_cvars(&cargs->data[i], names_per_carg[i],
                                 names_count_per_carg[i], table)) {
            return false;
        }
    }

    return true;
}
#endif

#endif
