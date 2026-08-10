#ifndef ZOD_NGINE_INTERNAL_H
#define ZOD_NGINE_INTERNAL_H

#include <ngine.lib/cvar.h>

// Runs every registered extension's init_config against `cvars`. Called by
// zngine_init and config_priv_reload_from_file.
void extensions_priv_init_config(cvar_table *cvars);

#endif
