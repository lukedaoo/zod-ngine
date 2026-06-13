#ifndef STRING_VIEW_H_
#define STRING_VIEW_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *data;
    size_t      size;
} StringView;

StringView string_view_init(const char *data, size_t size);
void       string_view_print(StringView view);
bool       string_view_equals(StringView a, StringView b);

#define SV(data)    string_view_init(data, sizeof(data) - 1)
#define SV_STR(str) ((StringView){.data = (str), .size = strlen(str)})

#ifdef STRING_VIEW_IMPLEMENTATION
StringView string_view_init(const char *data, size_t size) {
    StringView view;
    view.data = data;
    view.size = size;
    return view;
}

void string_view_print(StringView view) {
    printf("%.*s", (int)view.size, view.data);
}

bool string_view_equals(StringView a, StringView b) {
    return a.size == b.size && memcmp(a.data, b.data, a.size) == 0;
}

#endif

#endif
