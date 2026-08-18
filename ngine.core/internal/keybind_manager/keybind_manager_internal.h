#ifndef KEYBIND_MANAGER_INTERNAL_H
#define KEYBIND_MANAGER_INTERNAL_H

#include <ngine.lib/action.h>
#include <ngine.lib/collections/array_list.h>

#include "../../keybind_manager.h"

typedef struct {
    int           context;
    int           key;
    int           trigger;
    action_handle action;
} keybind_manager_entry;

struct keybind_manager {
    array_list bindings;
};

#endif
