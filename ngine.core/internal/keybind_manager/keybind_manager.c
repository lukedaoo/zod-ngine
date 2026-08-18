#ifdef ZOD_NGINE_IMPLEMENTATION

#include <ngine.lib/keybind.h>
#include <ngine.lib/keybind_load.h>
#include <ngine.lib/log.h>

#include "../../action_manager.h"
#include "keybind_manager_internal.h"

#ifndef DEFAULT_KEYBIND_MANAGER_CAPACITY
#define DEFAULT_KEYBIND_MANAGER_CAPACITY 16
#endif

static void keybind_manager_warn_unresolved(const char *fn, const keybind_vocab *vocab,
                                            const keybind *kb) {
    const char *key_name =
         (vocab && vocab->key_to_name) ? vocab->key_to_name(kb->key) : NULL;
    log_warn(
         "keybind_manager.%s: key '%s' (context %d, trigger %d) -> unresolved action "
         "'%s'",
         fn, key_name ? key_name : "?", kb->context, kb->trigger, kb->action);
}

void keybind_manager_priv_init(keybind_manager *mgr) {
    if (!mgr) return;
    array_list_init(&mgr->bindings, DEFAULT_KEYBIND_MANAGER_CAPACITY,
                    sizeof(keybind_manager_entry));
}

void keybind_manager_priv_destroy(keybind_manager *mgr) {
    if (!mgr) return;
    array_list_deinit(&mgr->bindings);
}

static size_t keybind_manager_index_of(const keybind_manager *mgr, const int context,
                                       const int key, const int trigger) {
    for (size_t i = 0; i < mgr->bindings.header.size; i++) {
        const keybind_manager_entry *entry =
             (const keybind_manager_entry *)array_list_get(&mgr->bindings, i);
        if (entry->context == context && entry->key == key && entry->trigger == trigger)
            return i;
    }
    return (size_t)-1;
}

bool keybind_manager_priv_load(keybind_manager *mgr, const char *path,
                               const keybind_vocab  *vocab,
                               const action_manager *actions) {
    if (!mgr) return false;

    keybind_table scratch;
    if (!keybind_init(&scratch)) return false;
    if (!keybind_load(&scratch, path, vocab)) {
        keybind_deinit(&scratch);
        return false;
    }

    array_list resolved;
    array_list_init(&resolved, DEFAULT_KEYBIND_MANAGER_CAPACITY,
                    sizeof(keybind_manager_entry));

    for (size_t i = 0; i < keybind_count(&scratch); i++) {
        keybind kb;
        if (!keybind_lookup(&scratch, keybind_at(&scratch, i), &kb)) continue;

        const action_handle resolved_handle = action_manager_priv_resolve_by_name(
             actions, kb.context, kb.trigger, kb.action);
        if (resolved_handle == ACTION_HANDLE_INVALID)
            keybind_manager_warn_unresolved("load", vocab, &kb);

        keybind_manager_entry entry = {.context = kb.context,
                                       .key     = kb.key,
                                       .trigger = kb.trigger,
                                       .action  = resolved_handle};
        array_list_append(&resolved, &entry);
    }
    keybind_deinit(&scratch);

    array_list_deinit(&mgr->bindings);
    mgr->bindings = resolved;
    return true;
}

bool keybind_manager_priv_merge(keybind_manager *mgr, const char *path,
                                const keybind_vocab  *vocab,
                                const action_manager *actions) {
    if (!mgr) return false;

    keybind_table scratch;
    if (!keybind_init(&scratch)) return false;
    if (!keybind_merge(&scratch, path, vocab)) {
        keybind_deinit(&scratch);
        return false;
    }

    for (size_t i = 0; i < keybind_count(&scratch); i++) {
        keybind kb;
        if (!keybind_lookup(&scratch, keybind_at(&scratch, i), &kb)) continue;

        const action_handle resolved = action_manager_priv_resolve_by_name(
             actions, kb.context, kb.trigger, kb.action);
        if (resolved == ACTION_HANDLE_INVALID)
            keybind_manager_warn_unresolved("merge", vocab, &kb);

        const size_t index =
             keybind_manager_index_of(mgr, kb.context, kb.key, kb.trigger);
        if (index != (size_t)-1) {
            keybind_manager_entry *entry =
                 (keybind_manager_entry *)array_list_get(&mgr->bindings, index);
            entry->action = resolved;
        } else {
            keybind_manager_entry entry = {.context = kb.context,
                                           .key     = kb.key,
                                           .trigger = kb.trigger,
                                           .action  = resolved};
            array_list_append(&mgr->bindings, &entry);
        }
    }
    keybind_deinit(&scratch);
    return true;
}

bool keybind_manager_priv_set(keybind_manager *mgr, const action_manager *actions,
                              const int context, const int key, const int trigger,
                              const char *action_name) {
    if (!mgr || !action_name) return false;

    const action_handle resolved =
         action_manager_priv_resolve_by_name(actions, context, trigger, action_name);
    if (resolved == ACTION_HANDLE_INVALID) return false;

    const size_t index = keybind_manager_index_of(mgr, context, key, trigger);
    if (index != (size_t)-1) {
        keybind_manager_entry *entry =
             (keybind_manager_entry *)array_list_get(&mgr->bindings, index);
        entry->action = resolved;
    } else {
        keybind_manager_entry entry = {
             .context = context, .key = key, .trigger = trigger, .action = resolved};
        array_list_append(&mgr->bindings, &entry);
    }
    return true;
}

bool keybind_manager_priv_unset(keybind_manager *mgr, const int context, const int key,
                                const int trigger) {
    if (!mgr) return false;
    const size_t index = keybind_manager_index_of(mgr, context, key, trigger);
    if (index == (size_t)-1) return false;
    return array_list_remove(&mgr->bindings, index);
}

action_handle keybind_manager_priv_resolve(const keybind_manager *mgr, const int context,
                                           const int key, const int trigger) {
    if (!mgr) return ACTION_HANDLE_INVALID;

    const size_t index = keybind_manager_index_of(mgr, context, key, trigger);
    if (index == (size_t)-1) return ACTION_HANDLE_INVALID;

    const keybind_manager_entry *entry =
         (const keybind_manager_entry *)array_list_get(&mgr->bindings, index);
    return entry->action;
}

size_t keybind_manager_priv_count(const keybind_manager *mgr) {
    return mgr ? mgr->bindings.header.size : 0;
}

bool keybind_manager_priv_at(const keybind_manager *mgr, const size_t index,
                             keybind_manager_entry *out) {
    if (!mgr || !out) return false;
    const keybind_manager_entry *entry =
         (const keybind_manager_entry *)array_list_get(&mgr->bindings, index);
    if (!entry) return false;
    *out = *entry;
    return true;
}

#endif
