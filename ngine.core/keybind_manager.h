#ifndef ZOD_KEYBIND_MANAGER_H
#define ZOD_KEYBIND_MANAGER_H

#include <ngine.lib/action.h>
#include <ngine.lib/keybind.h>

#include "action_manager.h"

typedef struct keybind_manager keybind_manager;

typedef struct {
    int           context;
    int           key;
    int           trigger;
    action_handle action;
} keybind_manager_entry;

void keybind_manager_priv_init(keybind_manager *mgr);
void keybind_manager_priv_destroy(keybind_manager *mgr);

bool keybind_manager_priv_load(keybind_manager *mgr, const char *path,
                               const keybind_vocab *vocab, const action_manager *actions);
bool keybind_manager_priv_merge(keybind_manager *mgr, const char *path,
                                const keybind_vocab  *vocab,
                                const action_manager *actions);

action_handle keybind_manager_priv_resolve(const keybind_manager *mgr, int context,
                                           int key, int trigger);

size_t keybind_manager_priv_count(const keybind_manager *mgr);
bool   keybind_manager_priv_at(const keybind_manager *mgr, size_t index,
                               keybind_manager_entry *out);

#endif
