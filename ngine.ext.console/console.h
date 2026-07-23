#ifndef CONSOLE_H
#define CONSOLE_H

#include <SDL3/SDL.h>
#include <ngine.lib/types.h>

typedef enum zconsole_input_kind {
    ZCONSOLE_INPUT_TEXT,
    ZCONSOLE_INPUT_BACKSPACE,
    ZCONSOLE_INPUT_SUBMIT,
    ZCONSOLE_INPUT_LEFT,
    ZCONSOLE_INPUT_RIGHT,
    ZCONSOLE_INPUT_HISTORY_PREV,
    ZCONSOLE_INPUT_HISTORY_NEXT,
    ZCONSOLE_INPUT_SCROLL_UP,
    ZCONSOLE_INPUT_SCROLL_DOWN,
} zconsole_input_kind;

typedef struct zconsole_input_event {
    zconsole_input_kind kind;
    const char         *text;  // only valid when kind == ZCONSOLE_INPUT_TEXT
} zconsole_input_event;

void zconsole_ext_install(void);

void zconsole_input_handle(const SDL_Event *event);

bool zconsole_toggle(void);
bool zconsole_draw(void);
bool zconsole_destroy(void);
bool zconsole_visible(void);
void zconsole_write(const char *fmt, ...);
void zconsole_write_color(color4f color, const char *fmt, ...);
void zconsole_handle_event(zconsole_input_event event);

#endif
