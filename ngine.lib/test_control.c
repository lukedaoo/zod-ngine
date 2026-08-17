#include "../thirdparty/minunit.h"

#include <stdio.h>
#include <string.h>

#define LOG_IMPLEMENTATION
#include "log.h"

#define ARRAY_LIST_IMPLEMENTATION
#include "collections/array_list.h"

#define ACTION_IMPLEMENTATION
#include "action.h"

#define CONTROL_IMPLEMENTATION
#include "control.h"

enum { CTX_GAME = 1, CTX_MENU = 2 };
enum { TRIG_PRESSED = 0, TRIG_DOWN = 1, TRIG_RELEASED = 2 };

enum { KEY_A = 4, KEY_B = 5, KEY_C = 6, KEY_SPACE = 44 };

static int fake_key_from_name(const char *name) {
    if (strcmp(name, "A") == 0) return KEY_A;
    if (strcmp(name, "B") == 0) return KEY_B;
    if (strcmp(name, "C") == 0) return KEY_C;
    if (strcmp(name, "Space") == 0) return KEY_SPACE;
    return 0;
}

static const char *fake_key_to_name(int key) {
    switch (key) {
        case KEY_A:
            return "A";
        case KEY_B:
            return "B";
        case KEY_C:
            return "C";
        case KEY_SPACE:
            return "Space";
        default:
            return NULL;
    }
}

static const control_context fake_contexts[] = {
     {"game", CTX_GAME},
     {"menu", CTX_MENU},
};

static const control_vocab fake_vocab = {
     .contexts      = fake_contexts,
     .context_count = 2,
     .key_from_name = fake_key_from_name,
     .key_to_name   = fake_key_to_name,
};

static control_table table;

static void setup(void) { mu_check(control_init(&table)); }
static void teardown(void) { control_deinit(&table); }

static bool binding_is(const control_handle handle, const action_mode context,
                       const char *action, const int key,
                       const action_trigger_type trigger) {
    control_binding out;
    if (!control_lookup(&table, handle, &out)) return false;
    return out.context == context && strcmp(out.action, action) == 0 && out.key == key &&
           out.trigger == trigger;
}

static size_t live_count(void) {
    size_t live = 0;
    for (size_t i = 0; i < control_count(&table); i++)
        if (control_at(&table, i) != CONTROL_HANDLE_INVALID) live++;
    return live;
}

MU_TEST(test_set_returns_resolving_handle) {
    const control_handle h = control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);
    mu_check(h != CONTROL_HANDLE_INVALID);
    mu_check(binding_is(h, CTX_GAME, "jump", KEY_A, TRIG_PRESSED));
}

MU_TEST(test_set_null_table_returns_invalid) {
    mu_check(control_set(NULL, CTX_GAME, "jump", KEY_A, TRIG_PRESSED) ==
             CONTROL_HANDLE_INVALID);
}

MU_TEST(test_set_null_action_returns_invalid) {
    mu_check(control_set(&table, CTX_GAME, NULL, KEY_A, TRIG_PRESSED) ==
             CONTROL_HANDLE_INVALID);
}

MU_TEST(test_set_zero_key_returns_invalid) {
    mu_check(control_set(&table, CTX_GAME, "jump", 0, TRIG_PRESSED) ==
             CONTROL_HANDLE_INVALID);
    mu_assert_int_eq(0, (int)control_count(&table));
}

MU_TEST(test_set_oversized_action_name_returns_invalid) {
    char name[CONTROL_ACTION_MAX + 8];
    memset(name, 'x', sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';

    mu_check(control_set(&table, CTX_GAME, name, KEY_A, TRIG_PRESSED) ==
             CONTROL_HANDLE_INVALID);
    mu_assert_int_eq(0, (int)control_count(&table));
}

MU_TEST(test_set_action_name_at_max_length_accepted) {
    char name[CONTROL_ACTION_MAX];
    memset(name, 'x', sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';

    const control_handle h = control_set(&table, CTX_GAME, name, KEY_A, TRIG_PRESSED);
    mu_check(h != CONTROL_HANDLE_INVALID);
    mu_check(binding_is(h, CTX_GAME, name, KEY_A, TRIG_PRESSED));
}

MU_TEST(test_find_returns_invalid_when_absent) {
    control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);
    mu_check(control_find(&table, CTX_GAME, "crouch") == CONTROL_HANDLE_INVALID);
    mu_check(control_find(&table, CTX_MENU, "jump") == CONTROL_HANDLE_INVALID);
}

MU_TEST(test_find_by_key_matches_context_key_and_trigger) {
    const control_handle h = control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_DOWN);

    mu_check(control_find_by_key(&table, CTX_GAME, KEY_A, TRIG_DOWN) == h);
    mu_check(control_find_by_key(&table, CTX_GAME, KEY_A, TRIG_PRESSED) ==
             CONTROL_HANDLE_INVALID);
    mu_check(control_find_by_key(&table, CTX_MENU, KEY_A, TRIG_DOWN) ==
             CONTROL_HANDLE_INVALID);
}

MU_TEST(test_lookup_rejects_invalid_handle) {
    control_binding out;
    control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);

    mu_check(!control_lookup(&table, CONTROL_HANDLE_INVALID, &out));
    mu_check(!control_lookup(&table, 99, &out));
    mu_check(!control_lookup(NULL, 1, &out));
    mu_check(!control_lookup(&table, 1, NULL));
}

MU_TEST(test_null_table_reads_are_safe) {
    mu_assert_int_eq(0, (int)control_count(NULL));
    mu_check(control_find(NULL, CTX_GAME, "jump") == CONTROL_HANDLE_INVALID);
    mu_check(control_find(&table, CTX_GAME, NULL) == CONTROL_HANDLE_INVALID);
    mu_check(control_find_by_key(NULL, CTX_GAME, KEY_A, TRIG_PRESSED) ==
             CONTROL_HANDLE_INVALID);
    mu_check(control_at(NULL, 0) == CONTROL_HANDLE_INVALID);
    mu_check(!control_unset(NULL, 1));
    mu_check(!control_init(NULL));
    mu_check(!control_clear(NULL));
    control_deinit(NULL);
}

MU_TEST(test_same_action_overwrites_in_place) {
    const control_handle first =
         control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);
    const control_handle again = control_set(&table, CTX_GAME, "jump", KEY_B, TRIG_DOWN);

    mu_check(first == again);
    mu_assert_int_eq(1, (int)control_count(&table));
    mu_check(binding_is(first, CTX_GAME, "jump", KEY_B, TRIG_DOWN));
}

MU_TEST(test_same_key_same_context_and_trigger_is_refused) {
    const control_handle first =
         control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);
    const control_handle second =
         control_set(&table, CTX_GAME, "crouch", KEY_A, TRIG_PRESSED);

    mu_check(second == CONTROL_HANDLE_INVALID);
    mu_assert_int_eq(1, (int)control_count(&table));
    mu_check(binding_is(first, CTX_GAME, "jump", KEY_A, TRIG_PRESSED));
}

MU_TEST(test_same_key_different_contexts_both_kept) {
    const control_handle game =
         control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);
    const control_handle menu =
         control_set(&table, CTX_MENU, "confirm", KEY_A, TRIG_PRESSED);

    mu_check(game != CONTROL_HANDLE_INVALID);
    mu_check(menu != CONTROL_HANDLE_INVALID);
    mu_check(game != menu);
    mu_assert_int_eq(2, (int)live_count());
}

MU_TEST(test_same_key_different_triggers_both_kept) {
    const control_handle press =
         control_set(&table, CTX_GAME, "tap", KEY_A, TRIG_PRESSED);
    const control_handle hold = control_set(&table, CTX_GAME, "hold", KEY_A, TRIG_DOWN);

    mu_check(press != CONTROL_HANDLE_INVALID);
    mu_check(hold != CONTROL_HANDLE_INVALID);
    mu_assert_int_eq(2, (int)live_count());
}

MU_TEST(test_rebind_past_own_row_is_accepted) {
    const control_handle jump =
         control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);
    control_set(&table, CTX_GAME, "crouch", KEY_B, TRIG_PRESSED);

    const control_handle again =
         control_set(&table, CTX_GAME, "jump", KEY_C, TRIG_PRESSED);
    mu_check(again == jump);
    mu_check(binding_is(jump, CTX_GAME, "jump", KEY_C, TRIG_PRESSED));
}

MU_TEST(test_rebind_onto_taken_key_is_refused_not_overwritten) {
    const control_handle crouch =
         control_set(&table, CTX_GAME, "crouch", KEY_B, TRIG_PRESSED);
    const control_handle jump =
         control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);

    mu_check(control_set(&table, CTX_GAME, "jump", KEY_B, TRIG_PRESSED) ==
             CONTROL_HANDLE_INVALID);
    mu_check(binding_is(jump, CTX_GAME, "jump", KEY_A, TRIG_PRESSED));
    mu_check(binding_is(crouch, CTX_GAME, "crouch", KEY_B, TRIG_PRESSED));
}

MU_TEST(test_unset_stops_resolution) {
    const control_handle h = control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);

    mu_check(control_unset(&table, h));
    control_binding out;
    mu_check(!control_lookup(&table, h, &out));
    mu_check(control_find(&table, CTX_GAME, "jump") == CONTROL_HANDLE_INVALID);
    mu_check(control_find_by_key(&table, CTX_GAME, KEY_A, TRIG_PRESSED) ==
             CONTROL_HANDLE_INVALID);
}

MU_TEST(test_double_unset_returns_false) {
    const control_handle h = control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);
    mu_check(control_unset(&table, h));
    mu_check(!control_unset(&table, h));
}

MU_TEST(test_unset_invalid_handle_returns_false) {
    control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);
    mu_check(!control_unset(&table, CONTROL_HANDLE_INVALID));
    mu_check(!control_unset(&table, 99));
}

MU_TEST(test_unrelated_unset_leaves_other_handles_valid) {
    const control_handle jump =
         control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);
    const control_handle crouch =
         control_set(&table, CTX_GAME, "crouch", KEY_B, TRIG_PRESSED);

    mu_check(control_unset(&table, crouch));
    mu_check(binding_is(jump, CTX_GAME, "jump", KEY_A, TRIG_PRESSED));
}

MU_TEST(test_unset_slot_is_reused_before_appending) {
    control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);
    const control_handle crouch =
         control_set(&table, CTX_GAME, "crouch", KEY_B, TRIG_PRESSED);
    mu_check(control_unset(&table, crouch));

    control_set(&table, CTX_GAME, "shoot", KEY_C, TRIG_PRESSED);
    mu_assert_int_eq(2, (int)control_count(&table));
    mu_assert_int_eq(2, (int)live_count());
}

MU_TEST(test_stale_handle_resolves_to_reused_slot) {
    control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);
    const control_handle crouch =
         control_set(&table, CTX_GAME, "crouch", KEY_B, TRIG_PRESSED);
    mu_check(control_unset(&table, crouch));

    control_set(&table, CTX_GAME, "shoot", KEY_C, TRIG_PRESSED);
    mu_check(binding_is(crouch, CTX_GAME, "shoot", KEY_C, TRIG_PRESSED));
}

MU_TEST(test_handle_survives_multiple_reallocations) {
    char                 name[CONTROL_ACTION_MAX];
    const control_handle first =
         control_set(&table, CTX_GAME, "first", KEY_A, TRIG_PRESSED);

    // initial capacity 16, growth 1.5 -> reallocates at 16, 24, 36
    for (int i = 0; i < 60; i++) {
        snprintf(name, sizeof(name), "filler_%d", i);
        mu_check(control_set(&table, CTX_MENU, name, 1000 + i, TRIG_PRESSED) !=
                 CONTROL_HANDLE_INVALID);
    }

    mu_check(binding_is(first, CTX_GAME, "first", KEY_A, TRIG_PRESSED));
    mu_check(control_find(&table, CTX_GAME, "first") == first);
}

MU_TEST(test_at_visits_every_live_binding_once) {
    control_set(&table, CTX_GAME, "a", KEY_A, TRIG_PRESSED);
    const control_handle b = control_set(&table, CTX_GAME, "b", KEY_B, TRIG_PRESSED);
    control_set(&table, CTX_GAME, "c", KEY_C, TRIG_PRESSED);
    mu_check(control_unset(&table, b));

    int seen_a = 0, seen_b = 0, seen_c = 0;
    for (size_t i = 0; i < control_count(&table); i++) {
        const control_handle h = control_at(&table, i);
        if (h == CONTROL_HANDLE_INVALID) continue;

        control_binding out;
        mu_check(control_lookup(&table, h, &out));
        if (strcmp(out.action, "a") == 0) seen_a++;
        if (strcmp(out.action, "b") == 0) seen_b++;
        if (strcmp(out.action, "c") == 0) seen_c++;
    }

    mu_assert_int_eq(1, seen_a);
    mu_assert_int_eq(0, seen_b);
    mu_assert_int_eq(1, seen_c);
}

MU_TEST(test_at_out_of_range_returns_invalid) {
    control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);
    mu_check(control_at(&table, 1) == CONTROL_HANDLE_INVALID);
    mu_check(control_at(&table, 999) == CONTROL_HANDLE_INVALID);
}

MU_TEST(test_clear_empties_the_table) {
    control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);
    mu_check(control_clear(&table));
    mu_assert_int_eq(0, (int)control_count(&table));
    mu_check(control_find(&table, CTX_GAME, "jump") == CONTROL_HANDLE_INVALID);
}

MU_TEST(test_key_to_string_round_trips) {
    mu_assert_string_eq("Space", control_key_to_string(&fake_vocab, KEY_SPACE));
    mu_assert_int_eq(KEY_SPACE, fake_key_from_name("Space"));
    mu_check(control_key_to_string(&fake_vocab, 9999) == NULL);
}

MU_TEST(test_builtin_trigger_names) {
    action_trigger_type trigger = -1;

    mu_check(control_trigger_from_string(&fake_vocab, "pressed", &trigger));
    mu_assert_int_eq(TRIG_PRESSED, trigger);
    mu_check(control_trigger_from_string(&fake_vocab, "down", &trigger));
    mu_assert_int_eq(TRIG_DOWN, trigger);
    mu_check(control_trigger_from_string(&fake_vocab, "released", &trigger));
    mu_assert_int_eq(TRIG_RELEASED, trigger);

    mu_check(!control_trigger_from_string(&fake_vocab, "sideways", &trigger));
    mu_check(!control_trigger_from_string(&fake_vocab, NULL, &trigger));
    mu_check(!control_trigger_from_string(&fake_vocab, "down", NULL));

    mu_assert_string_eq("down", control_trigger_to_string(&fake_vocab, TRIG_DOWN));
    mu_check(control_trigger_to_string(&fake_vocab, 42) == NULL);
}

MU_TEST_SUITE(control_suite) {
    MU_SUITE_CONFIGURE(&setup, &teardown);

    MU_RUN_TEST(test_set_returns_resolving_handle);
    MU_RUN_TEST(test_set_null_table_returns_invalid);
    MU_RUN_TEST(test_set_null_action_returns_invalid);
    MU_RUN_TEST(test_set_zero_key_returns_invalid);
    MU_RUN_TEST(test_set_oversized_action_name_returns_invalid);
    MU_RUN_TEST(test_set_action_name_at_max_length_accepted);
    MU_RUN_TEST(test_find_returns_invalid_when_absent);
    MU_RUN_TEST(test_find_by_key_matches_context_key_and_trigger);
    MU_RUN_TEST(test_lookup_rejects_invalid_handle);
    MU_RUN_TEST(test_null_table_reads_are_safe);
    MU_RUN_TEST(test_same_action_overwrites_in_place);
    MU_RUN_TEST(test_same_key_same_context_and_trigger_is_refused);
    MU_RUN_TEST(test_same_key_different_contexts_both_kept);
    MU_RUN_TEST(test_same_key_different_triggers_both_kept);
    MU_RUN_TEST(test_rebind_past_own_row_is_accepted);
    MU_RUN_TEST(test_rebind_onto_taken_key_is_refused_not_overwritten);
    MU_RUN_TEST(test_unset_stops_resolution);
    MU_RUN_TEST(test_double_unset_returns_false);
    MU_RUN_TEST(test_unset_invalid_handle_returns_false);
    MU_RUN_TEST(test_unrelated_unset_leaves_other_handles_valid);
    MU_RUN_TEST(test_unset_slot_is_reused_before_appending);
    MU_RUN_TEST(test_stale_handle_resolves_to_reused_slot);
    MU_RUN_TEST(test_handle_survives_multiple_reallocations);
    MU_RUN_TEST(test_at_visits_every_live_binding_once);
    MU_RUN_TEST(test_at_out_of_range_returns_invalid);
    MU_RUN_TEST(test_clear_empties_the_table);

    MU_RUN_TEST(test_key_to_string_round_trips);
    MU_RUN_TEST(test_builtin_trigger_names);
}

int main(void) {
    MU_RUN_SUITE(control_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
