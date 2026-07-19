#ifndef CONSOLE_H
#define CONSOLE_H

typedef enum console_input_kind {
    CONSOLE_INPUT_TEXT,
    CONSOLE_INPUT_BACKSPACE,
    CONSOLE_INPUT_SUBMIT,
    CONSOLE_INPUT_LEFT,
    CONSOLE_INPUT_RIGHT,
} console_input_kind;

typedef struct console_input_event {
    console_input_kind kind;
    const char        *text;  // only valid when kind == CONSOLE_INPUT_TEXT
} console_input_event;

void console_ext_install(void);

bool console_toggle(void);
bool console_draw(void);
bool console_destroy(void);
bool console_visible(void);
void console_write(const char *fmt, ...);
void console_handle_event(console_input_event event);

#endif
