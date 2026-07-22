#ifndef CONSOLE_H
#define CONSOLE_H

#include <ngine.lib/types.h>

typedef enum zconsole_input_kind {
    ZCONSOLE_INPUT_TEXT,
    ZCONSOLE_INPUT_BACKSPACE,
    ZCONSOLE_INPUT_SUBMIT,
    ZCONSOLE_INPUT_LEFT,
    ZCONSOLE_INPUT_RIGHT,
} zconsole_input_kind;

typedef struct zconsole_input_event {
    zconsole_input_kind kind;
    const char        *text;  // only valid when kind == ZCONSOLE_INPUT_TEXT
} zconsole_input_event;

void zconsole_ext_install(void);

bool zconsole_toggle(void);
bool zconsole_draw(void);
bool zconsole_destroy(void);
bool zconsole_visible(void);
void zconsole_write(const char *fmt, ...);
void zconsole_write_color(color4f color, const char *fmt, ...);
void zconsole_handle_event(zconsole_input_event event);

#endif
