#ifdef ZOD_NGINE_IMPLEMENTATION

#include <string.h>

#include <SDL3/SDL.h>

#include "../../keybind_alias.h"

static const struct {
    const char *alias;
    const char *sdl_name;
} keybind_sdl_aliases[] = {
     {"Grave", "`"},        {"Minus", "-"},      {"Equals", "="},    {"LeftBracket", "["},
     {"RightBracket", "]"}, {"Backslash", "\\"}, {"Semicolon", ";"}, {"Apostrophe", "'"},
     {"Comma", ","},        {"Period", "."},     {"Slash", "/"},
};

#define KEYBIND_SDL_ALIAS_COUNT \
    (sizeof(keybind_sdl_aliases) / sizeof(keybind_sdl_aliases[0]))

int zngine_key_from_name(const char *name) {
    if (!name) return 0;
    for (size_t i = 0; i < KEYBIND_SDL_ALIAS_COUNT; i++)
        if (strcmp(name, keybind_sdl_aliases[i].alias) == 0)
            return (int)SDL_GetScancodeFromName(keybind_sdl_aliases[i].sdl_name);
    return (int)SDL_GetScancodeFromName(name);
}

const char *zngine_key_to_name(const int key) {
    const char *sdl_name = SDL_GetScancodeName((SDL_Scancode)key);
    if (!sdl_name || !*sdl_name) return NULL;
    for (size_t i = 0; i < KEYBIND_SDL_ALIAS_COUNT; i++)
        if (strcmp(sdl_name, keybind_sdl_aliases[i].sdl_name) == 0)
            return keybind_sdl_aliases[i].alias;
    return sdl_name;
}

#endif
