#include "../../lib/minunit.h"

#define ARRAY_LIST_IMPLEMENTATION
#include "array_list.h"

typedef struct {
    char buf[512];
    int  id;
} large_elem;

MU_TEST(test_init_null_list) { mu_check(!array_list_init(NULL, 10, sizeof(int))); }

MU_TEST(test_init_zero_capacity) {
    array_list list = {0};
    mu_check(!array_list_init(&list, 0, sizeof(int)));
}

MU_TEST(test_init_zero_element_size) {
    array_list list = {0};
    mu_check(!array_list_init(&list, 16, 0));
}

MU_TEST(test_init_over_max_clamps) {
    array_list list = {0};
    mu_check(array_list_init(&list, ARRAY_LIST_MAX_CAPACITY + 100, sizeof(int)));
    mu_assert_int_eq(ARRAY_LIST_MAX_CAPACITY, (int)list.header.capacity);
    mu_assert_int_eq(0, (int)list.header.size);
    free(list.data);
}

MU_TEST(test_init_valid) {
    array_list list = {0};
    mu_check(array_list_init(&list, 10, sizeof(int)));
    mu_assert_int_eq(0, (int)list.header.size);
    mu_assert_int_eq(10, (int)list.header.capacity);
    mu_assert_int_eq((int)sizeof(int), (int)list.header.element_size);
    mu_check(list.data != NULL);
    mu_check(array_list_get(&list, 0) == NULL);
    free(list.data);
}

MU_TEST(test_init_valid_struct) {
    typedef struct {
        int   x;
        float y;
    } point;
    array_list list = {0};
    mu_check(array_list_init(&list, 4, sizeof(point)));
    mu_assert_int_eq((int)sizeof(point), (int)list.header.element_size);
    mu_check(list.data != NULL);
    free(list.data);
}

MU_TEST(test_create_valid) {
    array_list *list = array_list_create(10, sizeof(int));
    mu_check(list != NULL);
    mu_assert_int_eq(0, (int)list->header.size);
    mu_assert_int_eq(10, (int)list->header.capacity);
    mu_assert_int_eq((int)sizeof(int), (int)list->header.element_size);
    mu_check(list->data != NULL);
    array_list_destroy(list);
}

MU_TEST(test_create_zero_capacity_returns_null) {
    mu_check(array_list_create(0, sizeof(int)) == NULL);
}

MU_TEST(test_create_zero_elem_size_returns_null) {
    mu_check(array_list_create(16, 0) == NULL);
}

MU_TEST(test_destroy_null_safe) { array_list_destroy(NULL); }

MU_TEST(test_append_null_list) {
    int v = 1;
    mu_check(!array_list_append(NULL, &v));
}

MU_TEST(test_append_null_element) {
    array_list *list = array_list_create(4, sizeof(int));
    mu_check(!array_list_append(list, NULL));
    array_list_destroy(list);
}

MU_TEST(test_append_ints) {
    array_list *list    = array_list_create(4, sizeof(int));
    int         vals[3] = {10, 20, 30};
    for (int i = 0; i < 3; i++) mu_check(array_list_append(list, &vals[i]));
    mu_assert_int_eq(3, (int)list->header.size);
    mu_assert_int_eq(10, *arraylist_get_as(list, 0, int));
    mu_assert_int_eq(20, *arraylist_get_as(list, 1, int));
    mu_assert_int_eq(30, *arraylist_get_as(list, 2, int));
    array_list_destroy(list);
}

MU_TEST(test_append_struct) {
    typedef struct {
        int   id;
        float val;
    } item;
    array_list *list = array_list_create(4, sizeof(item));
    item        a    = {.id = 42, .val = 3.14F};
    mu_check(array_list_append(list, &a));
    item *got = (item *)array_list_get(list, 0);
    mu_check(got != NULL);
    mu_assert_int_eq(42, got->id);
    mu_check(got->val == 3.14F);
    array_list_destroy(list);
}

MU_TEST(test_append_triggers_growth) {
    array_list *list = array_list_create(4, sizeof(int));
    for (int i = 0; i < 10; i++) mu_check(array_list_append(list, &i));
    mu_assert_int_eq(10, (int)list->header.size);
    mu_check(list->header.capacity >= 10);
    for (int i = 0; i < 10; i++)
        mu_assert_int_eq(i, *arraylist_get_as(list, (size_t)i, int));
    array_list_destroy(list);
}

MU_TEST(test_append_fills_to_max) {
    array_list *list = array_list_create(16, sizeof(int));
    for (int i = 0; i < ARRAY_LIST_MAX_CAPACITY; i++)
        mu_check(array_list_append(list, &i));
    mu_assert_int_eq(ARRAY_LIST_MAX_CAPACITY, (int)list->header.size);
    mu_assert_int_eq(0, *arraylist_get_as(list, 0, int));
    mu_assert_int_eq(ARRAY_LIST_MAX_CAPACITY - 1,
                     *arraylist_get_as(list, ARRAY_LIST_MAX_CAPACITY - 1, int));
    array_list_destroy(list);
}

MU_TEST(test_remove_null_list) { mu_check(!array_list_remove(NULL, 0)); }

MU_TEST(test_remove_empty_list) {
    array_list *list = array_list_create(4, sizeof(int));
    mu_check(!array_list_remove(list, 0));
    array_list_destroy(list);
}

MU_TEST(test_remove_oob_eq_size) {
    array_list *list = array_list_create(4, sizeof(int));
    int         v    = 1;
    array_list_append(list, &v);
    mu_check(!array_list_remove(list, 1));
    array_list_destroy(list);
}

MU_TEST(test_remove_oob_gt_size) {
    array_list *list = array_list_create(4, sizeof(int));
    int         v    = 1;
    array_list_append(list, &v);
    mu_check(!array_list_remove(list, 5));
    array_list_destroy(list);
}

MU_TEST(test_remove_last) {
    array_list *list    = array_list_create(4, sizeof(int));
    int         vals[3] = {10, 20, 30};
    for (int i = 0; i < 3; i++) array_list_append(list, &vals[i]);
    mu_check(array_list_remove(list, 2));
    mu_assert_int_eq(2, (int)list->header.size);
    mu_assert_int_eq(10, *arraylist_get_as(list, 0, int));
    mu_assert_int_eq(20, *arraylist_get_as(list, 1, int));
    array_list_destroy(list);
}

MU_TEST(test_remove_first) {
    array_list *list    = array_list_create(4, sizeof(int));
    int         vals[3] = {10, 20, 30};
    for (int i = 0; i < 3; i++) array_list_append(list, &vals[i]);
    mu_check(array_list_remove(list, 0));
    mu_assert_int_eq(2, (int)list->header.size);
    mu_assert_int_eq(20, *arraylist_get_as(list, 0, int));
    mu_assert_int_eq(30, *arraylist_get_as(list, 1, int));
    array_list_destroy(list);
}

MU_TEST(test_remove_middle) {
    array_list *list    = array_list_create(4, sizeof(int));
    int         vals[4] = {10, 20, 30, 40};
    for (int i = 0; i < 4; i++) array_list_append(list, &vals[i]);
    mu_check(array_list_remove(list, 1));
    mu_assert_int_eq(3, (int)list->header.size);
    mu_assert_int_eq(10, *arraylist_get_as(list, 0, int));
    mu_assert_int_eq(30, *arraylist_get_as(list, 1, int));
    mu_assert_int_eq(40, *arraylist_get_as(list, 2, int));
    array_list_destroy(list);
}

MU_TEST(test_set_null_list) {
    int v = 1;
    mu_check(!array_list_set(NULL, 0, &v));
}

MU_TEST(test_set_oob) {
    array_list *list = array_list_create(4, sizeof(int));
    int         v    = 99;
    array_list_append(list, &v);
    mu_check(!array_list_set(list, 1, &v));
    mu_check(!array_list_set(list, 5, &v));
    array_list_destroy(list);
}

MU_TEST(test_set_overwrites) {
    array_list *list    = array_list_create(4, sizeof(int));
    int         vals[3] = {10, 20, 30};
    for (int i = 0; i < 3; i++) array_list_append(list, &vals[i]);
    int new_val = 99;
    mu_check(array_list_set(list, 1, &new_val));
    mu_assert_int_eq(10, *arraylist_get_as(list, 0, int));
    mu_assert_int_eq(99, *arraylist_get_as(list, 1, int));
    mu_assert_int_eq(30, *arraylist_get_as(list, 2, int));
    array_list_destroy(list);
}

MU_TEST(test_get_null_list) { mu_check(array_list_get(NULL, 0) == NULL); }

MU_TEST(test_get_oob) {
    array_list *list = array_list_create(4, sizeof(int));
    int         v    = 1;
    array_list_append(list, &v);
    mu_check(array_list_get(list, 1) == NULL);
    mu_check(array_list_get(list, 100) == NULL);
    array_list_destroy(list);
}

MU_TEST(test_get_valid) {
    array_list *list = array_list_create(4, sizeof(int));
    int         v    = 42;
    array_list_append(list, &v);
    int *got = arraylist_get_as(list, 0, int);
    mu_check(got != NULL);
    mu_assert_int_eq(42, *got);
    array_list_destroy(list);
}

MU_TEST(test_reserve_null_list) { mu_check(!array_list_reserve(NULL, 100)); }

MU_TEST(test_reserve_reduction_fails) {
    array_list *list = array_list_create(16, sizeof(int));
    mu_check(!array_list_reserve(list, 8));
    mu_assert_int_eq(16, (int)list->header.capacity);
    array_list_destroy(list);
}

MU_TEST(test_reserve_same_cap_fails) {
    array_list *list = array_list_create(16, sizeof(int));
    mu_check(!array_list_reserve(list, 16));
    mu_assert_int_eq(16, (int)list->header.capacity);
    array_list_destroy(list);
}

MU_TEST(test_reserve_valid) {
    array_list *list = array_list_create(16, sizeof(int));
    mu_check(array_list_reserve(list, 64));
    mu_assert_int_eq(64, (int)list->header.capacity);
    array_list_destroy(list);
}

MU_TEST(test_reserve_beyond_max_clamps) {
    array_list *list = array_list_create(16, sizeof(int));
    mu_check(array_list_reserve(list, ARRAY_LIST_MAX_CAPACITY + 100));
    mu_assert_int_eq(ARRAY_LIST_MAX_CAPACITY, (int)list->header.capacity);
    array_list_destroy(list);
}

MU_TEST(test_clear_null_list) { mu_check(!array_list_clear(NULL)); }

MU_TEST(test_clear_resets_size) {
    array_list *list    = array_list_create(8, sizeof(int));
    int         vals[3] = {1, 2, 3};
    for (int i = 0; i < 3; i++) array_list_append(list, &vals[i]);
    mu_check(array_list_clear(list));
    mu_assert_int_eq(0, (int)list->header.size);
    mu_assert_int_eq(8, (int)list->header.capacity);
    mu_check(list->data != NULL);
    array_list_destroy(list);
}

MU_TEST(test_clear_get_returns_null) {
    array_list *list = array_list_create(4, sizeof(int));
    int         v    = 42;
    array_list_append(list, &v);
    array_list_clear(list);
    mu_check(array_list_get(list, 0) == NULL);
    array_list_destroy(list);
}

MU_TEST(test_clear_allows_reuse) {
    array_list *list = array_list_create(4, sizeof(int));
    int         v    = 10;
    array_list_append(list, &v);
    array_list_clear(list);
    v = 99;
    mu_check(array_list_append(list, &v));
    mu_assert_int_eq(1, (int)list->header.size);
    mu_assert_int_eq(99, *arraylist_get_as(list, 0, int));
    array_list_destroy(list);
}

MU_TEST(test_clear_empty_list) {
    array_list *list = array_list_create(4, sizeof(int));
    mu_check(array_list_clear(list));
    mu_assert_int_eq(0, (int)list->header.size);
    array_list_destroy(list);
}

MU_TEST(test_shrink_null_list) { mu_check(!array_list_shrink(NULL)); }

MU_TEST(test_shrink_empty_list) {
    array_list *list = array_list_create(16, sizeof(int));
    mu_check(!array_list_shrink(list));
    mu_assert_int_eq(16, (int)list->header.capacity);
    array_list_destroy(list);
}

MU_TEST(test_shrink_already_fit) {
    array_list *list    = array_list_create(4, sizeof(int));
    int         vals[4] = {1, 2, 3, 4};
    for (int i = 0; i < 4; i++) array_list_append(list, &vals[i]);
    mu_assert_int_eq(4, (int)list->header.capacity);
    mu_check(!array_list_shrink(list));
    array_list_destroy(list);
}

MU_TEST(test_shrink_reduces_capacity) {
    array_list *list    = array_list_create(32, sizeof(int));
    int         vals[5] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) array_list_append(list, &vals[i]);
    mu_check(array_list_shrink(list));
    mu_assert_int_eq(5, (int)list->header.capacity);
    mu_assert_int_eq(5, (int)list->header.size);
    mu_assert_int_eq(10, *arraylist_get_as(list, 0, int));
    mu_assert_int_eq(50, *arraylist_get_as(list, 4, int));
    array_list_destroy(list);
}

MU_TEST(test_shrink_then_append_grows) {
    array_list *list = array_list_create(32, sizeof(int));
    int         v    = 7;
    array_list_append(list, &v);
    array_list_shrink(list);
    mu_assert_int_eq(1, (int)list->header.capacity);
    v = 8;
    mu_check(array_list_append(list, &v));
    mu_assert_int_eq(2, (int)list->header.size);
    mu_check(list->header.capacity > 1);
    array_list_destroy(list);
}

MU_TEST(test_stress_append_remove_alternating) {
    array_list *list = array_list_create(8, sizeof(int));
    for (int round = 0; round < 100; round++) {
        int v = round;
        mu_check(array_list_append(list, &v));
        mu_check(array_list_append(list, &v));
        mu_check(array_list_remove(list, 0));
    }
    mu_assert_int_eq(100, (int)list->header.size);
    array_list_destroy(list);
}

MU_TEST(test_stress_mixed_ops) {
    array_list *list = array_list_create(16, sizeof(int));
    for (int i = 0; i < 512; i++) mu_check(array_list_append(list, &i));
    mu_assert_int_eq(512, (int)list->header.size);
    for (int i = 0; i < 512; i += 2) {
        int v = -i;
        mu_check(array_list_set(list, (size_t)i, &v));
    }
    mu_assert_int_eq(0, *arraylist_get_as(list, 0, int));
    mu_assert_int_eq(1, *arraylist_get_as(list, 1, int));
    mu_assert_int_eq(-2, *arraylist_get_as(list, 2, int));
    for (int i = 0; i < 10; i++) mu_check(array_list_remove(list, 0));
    mu_assert_int_eq(502, (int)list->header.size);
    array_list_destroy(list);
}

MU_TEST(test_stress_large_element) {
    array_list *list = array_list_create(8, sizeof(large_elem));
    for (int i = 0; i < 100; i++) {
        large_elem elem = {0};
        elem.id         = i;
        elem.buf[0]     = (char)(i & 0xFF);
        mu_check(array_list_append(list, &elem));
    }
    mu_assert_int_eq(100, (int)list->header.size);
    large_elem *first = (large_elem *)array_list_get(list, 0);
    large_elem *last  = (large_elem *)array_list_get(list, 99);
    mu_check(first != NULL && first->id == 0);
    mu_check(last != NULL && last->id == 99);
    array_list_destroy(list);
}

MU_TEST(test_stress_growth_sequence) {
    array_list *list     = array_list_create(4, sizeof(int));
    size_t      last_cap = list->header.capacity;
    for (int i = 0; i < 512; i++) {
        mu_check(array_list_append(list, &i));
        size_t cap = list->header.capacity;
        mu_check(cap >= last_cap);
        mu_check(cap <= ARRAY_LIST_MAX_CAPACITY);
        last_cap = cap;
    }
    mu_assert_int_eq(512, (int)list->header.size);
    array_list_destroy(list);
}

MU_TEST_SUITE(array_list_suite) {
    MU_RUN_TEST(test_init_null_list);
    MU_RUN_TEST(test_init_zero_capacity);
    MU_RUN_TEST(test_init_zero_element_size);
    MU_RUN_TEST(test_init_over_max_clamps);
    MU_RUN_TEST(test_init_valid);
    MU_RUN_TEST(test_init_valid_struct);
    MU_RUN_TEST(test_create_valid);
    MU_RUN_TEST(test_create_zero_capacity_returns_null);
    MU_RUN_TEST(test_create_zero_elem_size_returns_null);
    MU_RUN_TEST(test_destroy_null_safe);
    MU_RUN_TEST(test_append_null_list);
    MU_RUN_TEST(test_append_null_element);
    MU_RUN_TEST(test_append_ints);
    MU_RUN_TEST(test_append_struct);
    MU_RUN_TEST(test_append_triggers_growth);
    MU_RUN_TEST(test_append_fills_to_max);
    MU_RUN_TEST(test_remove_null_list);
    MU_RUN_TEST(test_remove_empty_list);
    MU_RUN_TEST(test_remove_oob_eq_size);
    MU_RUN_TEST(test_remove_oob_gt_size);
    MU_RUN_TEST(test_remove_last);
    MU_RUN_TEST(test_remove_first);
    MU_RUN_TEST(test_remove_middle);
    MU_RUN_TEST(test_set_null_list);
    MU_RUN_TEST(test_set_oob);
    MU_RUN_TEST(test_set_overwrites);
    MU_RUN_TEST(test_get_null_list);
    MU_RUN_TEST(test_get_oob);
    MU_RUN_TEST(test_get_valid);
    MU_RUN_TEST(test_reserve_null_list);
    MU_RUN_TEST(test_reserve_reduction_fails);
    MU_RUN_TEST(test_reserve_same_cap_fails);
    MU_RUN_TEST(test_reserve_valid);
    MU_RUN_TEST(test_reserve_beyond_max_clamps);
    MU_RUN_TEST(test_clear_null_list);
    MU_RUN_TEST(test_clear_resets_size);
    MU_RUN_TEST(test_clear_get_returns_null);
    MU_RUN_TEST(test_clear_allows_reuse);
    MU_RUN_TEST(test_clear_empty_list);
    MU_RUN_TEST(test_shrink_null_list);
    MU_RUN_TEST(test_shrink_empty_list);
    MU_RUN_TEST(test_shrink_already_fit);
    MU_RUN_TEST(test_shrink_reduces_capacity);
    MU_RUN_TEST(test_shrink_then_append_grows);
    MU_RUN_TEST(test_stress_append_remove_alternating);
    MU_RUN_TEST(test_stress_mixed_ops);
    MU_RUN_TEST(test_stress_large_element);
    MU_RUN_TEST(test_stress_growth_sequence);
}

int main(void) {
    MU_RUN_SUITE(array_list_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
