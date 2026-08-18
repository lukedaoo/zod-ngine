#ifndef KEYBIND_TO_ACTION_H
#define KEYBIND_TO_ACTION_H

#include "action.h"
#include "keybind.h"

action_handle keybind_to_action(const keybind_table *binds, const action_table *actions,
                                const int context, const int key, const int trigger);

#ifdef KEYBIND_TO_ACTION_IMPLEMENTATION

action_handle keybind_to_action(const keybind_table *binds, const action_table *actions,
                                const int context, const int key, const int trigger) {
    const keybind_handle handle = keybind_find_by_key(binds, context, key, trigger);
    if (handle == KEYBIND_HANDLE_INVALID) return ACTION_HANDLE_INVALID;

    keybind entry;
    if (!keybind_lookup(binds, handle, &entry)) return ACTION_HANDLE_INVALID;

    return action_resolve_by_name(actions, context, trigger, entry.action);
}

#endif
#endif
