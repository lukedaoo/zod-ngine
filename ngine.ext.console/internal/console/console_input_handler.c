#ifdef ZOD_NGINE_IMPLEMENTATION
#include <SDL3/SDL.h>

#include "../../console.h"

void zconsole_input_handle(const SDL_Event *e) {
    if (e->type == SDL_EVENT_TEXT_INPUT) {
        if (strcmp(e->text.text, "~") == 0 || strcmp(e->text.text, "`") == 0) return;
        zconsole_handle_event(
             (zconsole_input_event){.kind = ZCONSOLE_INPUT_TEXT, .text = e->text.text});
    } else if (e->type == SDL_EVENT_KEY_DOWN) {
        if (e->key.key == SDLK_BACKSPACE) {
            zconsole_handle_event(
                 (zconsole_input_event){.kind = ZCONSOLE_INPUT_BACKSPACE});
        } else if (e->key.key == SDLK_RETURN || e->key.key == SDLK_KP_ENTER) {
            zconsole_handle_event((zconsole_input_event){.kind = ZCONSOLE_INPUT_SUBMIT});
        } else if (e->key.key == SDLK_LEFT) {
            zconsole_handle_event((zconsole_input_event){.kind = ZCONSOLE_INPUT_LEFT});
        } else if (e->key.key == SDLK_RIGHT) {
            zconsole_handle_event((zconsole_input_event){.kind = ZCONSOLE_INPUT_RIGHT});
        } else if (e->key.key == SDLK_UP) {
            zconsole_handle_event(
                 (zconsole_input_event){.kind = ZCONSOLE_INPUT_HISTORY_PREV});
        } else if (e->key.key == SDLK_DOWN) {
            zconsole_handle_event(
                 (zconsole_input_event){.kind = ZCONSOLE_INPUT_HISTORY_NEXT});
        } else if (e->key.key == SDLK_PAGEUP) {
            zconsole_handle_event(
                 (zconsole_input_event){.kind = ZCONSOLE_INPUT_SCROLL_UP});
        } else if (e->key.key == SDLK_PAGEDOWN) {
            zconsole_handle_event(
                 (zconsole_input_event){.kind = ZCONSOLE_INPUT_SCROLL_DOWN});
        }
    }
}

#endif
