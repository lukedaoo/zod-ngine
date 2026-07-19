#ifndef STRING_VIEW_H_
#define STRING_VIEW_H_

#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct string_view string_view;

string_view string_view_init(const char *data, size_t size);
void        string_view_print(string_view view);
bool        string_view_equals(string_view a, string_view b);

#define SV(data)    string_view_init(data, sizeof(data) - 1)
#define SV_STR(str) ((string_view){.data = (str), .size = strlen(str)})

#ifdef STRING_VIEW_IMPLEMENTATION

struct string_view {
    const char *data;
    size_t      size;
};

string_view string_view_init(const char *data, size_t size) {
    string_view view;
    view.data = data;
    view.size = size;
    return view;
}

void string_view_print(string_view view) { printf("%.*s", (int)view.size, view.data); }

bool string_view_equals(string_view a, string_view b) {
    return a.size == b.size && memcmp(a.data, b.data, a.size) == 0;
}

#endif

#endif
