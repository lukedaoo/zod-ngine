#ifndef CONSOLE_H
#define CONSOLE_H

bool console_toggle(void);
bool console_draw(void);
bool console_destroy(void);
void console_write(const char *fmt, ...);

#endif
