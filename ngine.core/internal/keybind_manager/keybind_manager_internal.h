#ifndef KEYBIND_MANAGER_INTERNAL_H
#define KEYBIND_MANAGER_INTERNAL_H

#include <ngine.lib/action.h>
#include <ngine.lib/collections/array_list.h>

#include "../../keybind_manager.h"

struct keybind_manager {
    array_list bindings;
};

#endif
