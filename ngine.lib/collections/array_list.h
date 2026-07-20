#ifndef ARRAY_LIST_H
#define ARRAY_LIST_H

#include <stddef.h>
#include <stdbool.h>

#include "../log.h"

typedef struct array_list        array_list;
typedef struct array_list_header array_list_header;

bool        array_list_init(array_list *list, const size_t initial_capacity,
                            const size_t element_size);
array_list *array_list_create(const size_t initial_capacity, const size_t element_size);
// @info(array_list_clear): set the list size to zero, do not free memory
bool array_list_clear(array_list *list);
// @info(array_list_deinit): free the internal data buffer only, not the list
// struct itself. Use for stack/embedded lists from array_list_init.
void array_list_deinit(array_list *list);
// @info(array_list_destroy): free the data buffer and the list struct. Use for
// heap lists from array_list_create.
void array_list_destroy(array_list *list);
// @info(array_list_shrink): reduces the capacity to exact size
bool array_list_shrink(array_list *list);
// @info(array_list_reserve): ensure that the capacity is at least new_capacity
bool array_list_reserve(array_list *list, const size_t new_capacity);

bool array_list_append(array_list *list, const void *element);
bool array_list_remove(array_list *list, const size_t index);
bool array_list_set(array_list *list, const size_t index, const void *element);

void *array_list_get(const array_list *list, const size_t index);

#define arraylist_get_as(list, index, type)                                     \
    _Generic((*(type *)0),                                                      \
         int: (int *)array_list_get(list, index),                               \
         float: (float *)array_list_get(list, index),                           \
         double: (double *)array_list_get(list, index),                         \
         char: (char *)array_list_get(list, index),                             \
         short: (short *)array_list_get(list, index),                           \
         long: (long *)array_list_get(list, index),                             \
         long long: (long long *)array_list_get(list, index),                   \
         unsigned int: (unsigned int *)array_list_get(list, index),             \
         unsigned char: (unsigned char *)array_list_get(list, index),           \
         unsigned short: (unsigned short *)array_list_get(list, index),         \
         unsigned long: (unsigned long *)array_list_get(list, index),           \
         unsigned long long: (unsigned long long *)array_list_get(list, index), \
         char *: (char **)array_list_get(list, index),                          \
         void *: (void **)array_list_get(list, index),                          \
         default: (void *)array_list_get(list, index))

#ifdef ARRAY_LIST_IMPLEMENTATION
#include <stdlib.h>
#include <string.h>

#ifndef ARRAY_LIST_INITIAL_CAPACITY
#define ARRAY_LIST_INITIAL_CAPACITY 16
#endif

#ifndef ARRAY_LIST_GROWTH_FACTOR
#define ARRAY_LIST_GROWTH_FACTOR 1.5F
#endif

#ifndef ARRAY_LIST_MAX_CAPACITY
#define ARRAY_LIST_MAX_CAPACITY 2048
#endif

#ifndef ARRAY_LIST_LOG_ENABLED
#define ARRAY_LIST_LOG_ENABLED 0
#endif

struct array_list_header {
    size_t size;
    size_t capacity;
    size_t element_size;
};

struct array_list {
    array_list_header header;
    void             *data;
};

bool array_list_init(array_list *list, const size_t initial_capacity,
                     const size_t element_size) {
    if (!list || initial_capacity <= 0 || element_size <= 0) return false;

    size_t cap = initial_capacity;
    if (cap > ARRAY_LIST_MAX_CAPACITY) {
#if ARRAY_LIST_LOG_ENABLED
        log_warn(
             "array_list.array_list_init: capacity overflow, clamped to "
             "max capacity: %zu.",
             ARRAY_LIST_MAX_CAPACITY);
#endif
        cap = ARRAY_LIST_MAX_CAPACITY;
    }

    list->header = (array_list_header){
         .size         = 0,
         .capacity     = cap,
         .element_size = element_size  //
    };
    list->data = (void *)malloc(list->header.capacity * list->header.element_size);
    return list->data != NULL;
}

array_list *array_list_create(const size_t initial_capacity, const size_t element_size) {
    array_list *res = malloc(sizeof(array_list));
    if (!res) return NULL;

    if (!array_list_init(res, initial_capacity, element_size)) {
        free(res);
        return NULL;
    }

    return res;
}

bool array_list_reserve(array_list *list, const size_t new_capacity) {
    if (!list || new_capacity <= list->header.capacity) return false;
    size_t cap_to_reserve = new_capacity;
    if (new_capacity > ARRAY_LIST_MAX_CAPACITY) {
#if ARRAY_LIST_LOG_ENABLED
        log_warn(
             "array_list.array_list_reserve: capacity overflow, reserved with max "
             "capacity: %zu.",
             ARRAY_LIST_MAX_CAPACITY);
#endif
        cap_to_reserve = ARRAY_LIST_MAX_CAPACITY;
    }
    void *new_data = realloc(list->data, cap_to_reserve * list->header.element_size);
    if (!new_data) return false;
    list->data            = new_data;
    list->header.capacity = cap_to_reserve;
    return true;
}

bool array_list_clear(array_list *list) {
    if (!list) return false;
    list->header.size = 0;
    return true;
}

bool array_list_shrink(array_list *list) {
    if (!list || list->header.size == 0 || list->header.size >= list->header.capacity)
        return false;
    void *new_data = realloc(list->data, list->header.size * list->header.element_size);
    if (!new_data) return false;
    list->data            = new_data;
    list->header.capacity = list->header.size;
    return true;
}

void array_list_deinit(array_list *list) {
    if (!list) return;
    free(list->data);
    list->data            = NULL;
    list->header.size     = 0;
    list->header.capacity = 0;
}

void array_list_destroy(array_list *list) {
    if (!list) return;
    array_list_deinit(list);
    free(list);
}

bool array_list_append(array_list *list, const void *element) {
    if (!list || !element) return false;

    if (list->header.size >= list->header.capacity) {
        const float growth_factor = (float)ARRAY_LIST_GROWTH_FACTOR;
        size_t new_capacity = (size_t)((double)list->header.capacity * growth_factor);
        if (new_capacity <= list->header.capacity)
            new_capacity = list->header.capacity + 1;
        if (new_capacity > ARRAY_LIST_MAX_CAPACITY) {
#if ARRAY_LIST_LOG_ENABLED
            log_warn(
                 "array_list.array_list_append: capacity overflow. Max capacity: %zu.",
                 ARRAY_LIST_MAX_CAPACITY);
#endif
            new_capacity = ARRAY_LIST_MAX_CAPACITY;
        }
        void *new_data = realloc(list->data, new_capacity * list->header.element_size);
        if (!new_data) return false;

        list->data            = new_data;
        list->header.capacity = new_capacity;
    }
    void *target =
         (void *)((char *)list->data + list->header.size * list->header.element_size);
    memcpy(target, element, list->header.element_size);
    list->header.size++;
    return true;
}

bool array_list_remove(array_list *list, const size_t index) {
    if (!list || index >= list->header.size) return false;

    void *target = (void *)((char *)list->data + index * list->header.element_size);
    memmove(target, target + list->header.element_size,
            (list->header.size - index - 1) * list->header.element_size);
    list->header.size--;
    return true;
}

bool array_list_set(array_list *list, const size_t index, const void *element) {
    if (!list || index >= list->header.size) return false;
    void *target = (void *)((char *)list->data + index * list->header.element_size);
    memcpy(target, element, list->header.element_size);
    return true;
}

void *array_list_get(const array_list *list, const size_t index) {
    if (!list || index >= list->header.size) return NULL;
    return (void *)((char *)list->data + index * list->header.element_size);
}

#endif
#endif
