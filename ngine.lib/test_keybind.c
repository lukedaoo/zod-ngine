#include "../thirdparty/minunit.h"

#include <stdio.h>
#include <string.h>

#define LOG_IMPLEMENTATION
#include "log.h"

#define ARRAY_LIST_IMPLEMENTATION
#include "collections/array_list.h"

#define KEYBIND_IMPLEMENTATION
#include "keybind.h"

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

static const keybind_context fake_contexts[] = {
     {"game", CTX_GAME},
     {"menu", CTX_MENU},
};

static const keybind_vocab fake_vocab = {
     .contexts      = fake_contexts,
     .context_count = 2,
     .key_from_name = fake_key_from_name,
     .key_to_name   = fake_key_to_name,
};

static keybind_table table;

static void setup(void) { mu_check(keybind_init(&table)); }
static void teardown(void) { keybind_deinit(&table); }

static bool binding_is(const keybind_handle handle, const int context,
                       const char *action, const int key,
                       const int trigger) {
    keybind out;
    if (!keybind_lookup(&table, handle, &out)) return false;
    return out.context == context && strcmp(out.action, action) == 0 && out.key == key &&
           out.trigger == trigger;
}

static size_t live_count(void) {
    size_t live = 0;
    for (size_t i = 0; i < keybind_count(&table); i++)
        if (keybind_at(&table, i) != KEYBIND_HANDLE_INVALID) live++;
    return live;
}

MU_TEST(test_set_returns_resolving_handle) {
    const keybind_handle h = keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");
    mu_check(h != KEYBIND_HANDLE_INVALID);
    mu_check(binding_is(h, CTX_GAME, "jump", KEY_A, TRIG_PRESSED));
}

MU_TEST(test_set_null_table_returns_invalid) {
    mu_check(keybind_set(NULL, CTX_GAME, KEY_A, TRIG_PRESSED, "jump") ==
             KEYBIND_HANDLE_INVALID);
}

MU_TEST(test_set_null_action_returns_invalid) {
    mu_check(keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, NULL) ==
             KEYBIND_HANDLE_INVALID);
}

MU_TEST(test_set_zero_key_returns_invalid) {
    mu_check(keybind_set(&table, CTX_GAME, 0, TRIG_PRESSED, "jump") ==
             KEYBIND_HANDLE_INVALID);
    mu_assert_int_eq(0, (int)keybind_count(&table));
}

MU_TEST(test_set_oversized_action_name_returns_invalid) {
    char name[KEYBIND_ACTION_MAX + 8];
    memset(name, 'x', sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';

    mu_check(keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, name) ==
             KEYBIND_HANDLE_INVALID);
    mu_assert_int_eq(0, (int)keybind_count(&table));
}

MU_TEST(test_set_action_name_at_max_length_accepted) {
    char name[KEYBIND_ACTION_MAX];
    memset(name, 'x', sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';

    const keybind_handle h = keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, name);
    mu_check(h != KEYBIND_HANDLE_INVALID);
    mu_check(binding_is(h, CTX_GAME, name, KEY_A, TRIG_PRESSED));
}

MU_TEST(test_find_returns_invalid_when_absent) {
    keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");
    mu_check(keybind_find_action(&table, CTX_GAME, "crouch") == KEYBIND_HANDLE_INVALID);
    mu_check(keybind_find_action(&table, CTX_MENU, "jump") == KEYBIND_HANDLE_INVALID);
}

MU_TEST(test_find_by_key_matches_context_key_and_trigger) {
    const keybind_handle h = keybind_set(&table, CTX_GAME, KEY_A, TRIG_DOWN, "jump");

    mu_check(keybind_find_by_key(&table, CTX_GAME, KEY_A, TRIG_DOWN) == h);
    mu_check(keybind_find_by_key(&table, CTX_GAME, KEY_A, TRIG_PRESSED) ==
             KEYBIND_HANDLE_INVALID);
    mu_check(keybind_find_by_key(&table, CTX_MENU, KEY_A, TRIG_DOWN) ==
             KEYBIND_HANDLE_INVALID);
}

MU_TEST(test_lookup_rejects_invalid_handle) {
    keybind out;
    keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");

    mu_check(!keybind_lookup(&table, KEYBIND_HANDLE_INVALID, &out));
    mu_check(!keybind_lookup(&table, 99, &out));
    mu_check(!keybind_lookup(NULL, 1, &out));
    mu_check(!keybind_lookup(&table, 1, NULL));
}

MU_TEST(test_null_table_reads_are_safe) {
    mu_assert_int_eq(0, (int)keybind_count(NULL));
    mu_check(keybind_find_action(NULL, CTX_GAME, "jump") == KEYBIND_HANDLE_INVALID);
    mu_check(keybind_find_action(&table, CTX_GAME, NULL) == KEYBIND_HANDLE_INVALID);
    mu_check(keybind_find_by_key(NULL, CTX_GAME, KEY_A, TRIG_PRESSED) ==
             KEYBIND_HANDLE_INVALID);
    mu_check(keybind_at(NULL, 0) == KEYBIND_HANDLE_INVALID);
    mu_check(!keybind_unset(NULL, 1));
    mu_check(!keybind_init(NULL));
    mu_check(!keybind_clear(NULL));
    keybind_deinit(NULL);
}



MU_TEST(test_same_key_different_contexts_both_kept) {
    const keybind_handle game =
         keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");
    const keybind_handle menu =
         keybind_set(&table, CTX_MENU, KEY_A, TRIG_PRESSED, "confirm");

    mu_check(game != KEYBIND_HANDLE_INVALID);
    mu_check(menu != KEYBIND_HANDLE_INVALID);
    mu_check(game != menu);
    mu_assert_int_eq(2, (int)live_count());
}

MU_TEST(test_same_key_different_triggers_both_kept) {
    const keybind_handle press =
         keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "tap");
    const keybind_handle hold = keybind_set(&table, CTX_GAME, KEY_A, TRIG_DOWN, "hold");

    mu_check(press != KEYBIND_HANDLE_INVALID);
    mu_check(hold != KEYBIND_HANDLE_INVALID);
    mu_assert_int_eq(2, (int)live_count());
}



MU_TEST(test_unset_stops_resolution) {
    const keybind_handle h = keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");

    mu_check(keybind_unset(&table, h));
    keybind out;
    mu_check(!keybind_lookup(&table, h, &out));
    mu_check(keybind_find_action(&table, CTX_GAME, "jump") == KEYBIND_HANDLE_INVALID);
    mu_check(keybind_find_by_key(&table, CTX_GAME, KEY_A, TRIG_PRESSED) ==
             KEYBIND_HANDLE_INVALID);
}

MU_TEST(test_double_unset_returns_false) {
    const keybind_handle h = keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");
    mu_check(keybind_unset(&table, h));
    mu_check(!keybind_unset(&table, h));
}

MU_TEST(test_unset_invalid_handle_returns_false) {
    keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");
    mu_check(!keybind_unset(&table, KEYBIND_HANDLE_INVALID));
    mu_check(!keybind_unset(&table, 99));
}

MU_TEST(test_unrelated_unset_leaves_other_handles_valid) {
    const keybind_handle jump =
         keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");
    const keybind_handle crouch =
         keybind_set(&table, CTX_GAME, KEY_B, TRIG_PRESSED, "crouch");

    mu_check(keybind_unset(&table, crouch));
    mu_check(binding_is(jump, CTX_GAME, "jump", KEY_A, TRIG_PRESSED));
}

MU_TEST(test_unset_slot_is_reused_before_appending) {
    keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");
    const keybind_handle crouch =
         keybind_set(&table, CTX_GAME, KEY_B, TRIG_PRESSED, "crouch");
    mu_check(keybind_unset(&table, crouch));

    keybind_set(&table, CTX_GAME, KEY_C, TRIG_PRESSED, "shoot");
    mu_assert_int_eq(2, (int)keybind_count(&table));
    mu_assert_int_eq(2, (int)live_count());
}

MU_TEST(test_stale_handle_resolves_to_reused_slot) {
    keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");
    const keybind_handle crouch =
         keybind_set(&table, CTX_GAME, KEY_B, TRIG_PRESSED, "crouch");
    mu_check(keybind_unset(&table, crouch));

    keybind_set(&table, CTX_GAME, KEY_C, TRIG_PRESSED, "shoot");
    mu_check(binding_is(crouch, CTX_GAME, "shoot", KEY_C, TRIG_PRESSED));
}

MU_TEST(test_handle_survives_multiple_reallocations) {
    char                 name[KEYBIND_ACTION_MAX];
    const keybind_handle first =
         keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "first");

    // initial capacity 16, growth 1.5 -> reallocates at 16, 24, 36
    for (int i = 0; i < 60; i++) {
        snprintf(name, sizeof(name), "filler_%d", i);
        mu_check(keybind_set(&table, CTX_MENU, 1000 + i, TRIG_PRESSED, name) !=
                 KEYBIND_HANDLE_INVALID);
    }

    mu_check(binding_is(first, CTX_GAME, "first", KEY_A, TRIG_PRESSED));
    mu_check(keybind_find_action(&table, CTX_GAME, "first") == first);
}

MU_TEST(test_at_visits_every_live_binding_once) {
    keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "a");
    const keybind_handle b = keybind_set(&table, CTX_GAME, KEY_B, TRIG_PRESSED, "b");
    keybind_set(&table, CTX_GAME, KEY_C, TRIG_PRESSED, "c");
    mu_check(keybind_unset(&table, b));

    int seen_a = 0, seen_b = 0, seen_c = 0;
    for (size_t i = 0; i < keybind_count(&table); i++) {
        const keybind_handle h = keybind_at(&table, i);
        if (h == KEYBIND_HANDLE_INVALID) continue;

        keybind out;
        mu_check(keybind_lookup(&table, h, &out));
        if (strcmp(out.action, "a") == 0) seen_a++;
        if (strcmp(out.action, "b") == 0) seen_b++;
        if (strcmp(out.action, "c") == 0) seen_c++;
    }

    mu_assert_int_eq(1, seen_a);
    mu_assert_int_eq(0, seen_b);
    mu_assert_int_eq(1, seen_c);
}

MU_TEST(test_at_out_of_range_returns_invalid) {
    keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");
    mu_check(keybind_at(&table, 1) == KEYBIND_HANDLE_INVALID);
    mu_check(keybind_at(&table, 999) == KEYBIND_HANDLE_INVALID);
}

MU_TEST(test_clear_empties_the_table) {
    keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");
    mu_check(keybind_clear(&table));
    mu_assert_int_eq(0, (int)keybind_count(&table));
    mu_check(keybind_find_action(&table, CTX_GAME, "jump") == KEYBIND_HANDLE_INVALID);
}

MU_TEST(test_key_to_string_round_trips) {
    mu_assert_string_eq("Space", keybind_key_to_string(&fake_vocab, KEY_SPACE));
    mu_assert_int_eq(KEY_SPACE, fake_key_from_name("Space"));
    mu_check(keybind_key_to_string(&fake_vocab, 9999) == NULL);
}

MU_TEST(test_builtin_trigger_names) {
    int trigger = -1;

    mu_check(keybind_trigger_from_string(&fake_vocab, "pressed", &trigger));
    mu_assert_int_eq(TRIG_PRESSED, trigger);
    mu_check(keybind_trigger_from_string(&fake_vocab, "down", &trigger));
    mu_assert_int_eq(TRIG_DOWN, trigger);
    mu_check(keybind_trigger_from_string(&fake_vocab, "released", &trigger));
    mu_assert_int_eq(TRIG_RELEASED, trigger);

    mu_check(!keybind_trigger_from_string(&fake_vocab, "sideways", &trigger));
    mu_check(!keybind_trigger_from_string(&fake_vocab, NULL, &trigger));
    mu_check(!keybind_trigger_from_string(&fake_vocab, "down", NULL));

    mu_assert_string_eq("down", keybind_trigger_to_string(&fake_vocab, TRIG_DOWN));
    mu_check(keybind_trigger_to_string(&fake_vocab, 42) == NULL);
}

MU_TEST(test_same_key_reassigns_in_place) {
    const keybind_handle first =
         keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");
    const keybind_handle again =
         keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "crouch");

    mu_check(first == again);
    mu_assert_int_eq(1, (int)keybind_count(&table));
    mu_check(binding_is(first, CTX_GAME, "crouch", KEY_A, TRIG_PRESSED));
}

MU_TEST(test_one_action_on_several_keys) {
    const keybind_handle a = keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "move");
    const keybind_handle b = keybind_set(&table, CTX_GAME, KEY_B, TRIG_PRESSED, "move");

    mu_check(a != KEYBIND_HANDLE_INVALID);
    mu_check(b != KEYBIND_HANDLE_INVALID);
    mu_check(a != b);
    mu_assert_int_eq(2, (int)live_count());
    mu_check(binding_is(a, CTX_GAME, "move", KEY_A, TRIG_PRESSED));
    mu_check(binding_is(b, CTX_GAME, "move", KEY_B, TRIG_PRESSED));
}

MU_TEST(test_swapping_two_keys_works_in_either_order) {
    keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");
    keybind_set(&table, CTX_GAME, KEY_B, TRIG_PRESSED, "crouch");

    keybind_set(&table, CTX_GAME, KEY_B, TRIG_PRESSED, "jump");
    keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "crouch");

    mu_assert_int_eq(2, (int)live_count());
    mu_check(binding_is(keybind_find_by_key(&table, CTX_GAME, KEY_B, TRIG_PRESSED),
                        CTX_GAME, "jump", KEY_B, TRIG_PRESSED));
    mu_check(binding_is(keybind_find_by_key(&table, CTX_GAME, KEY_A, TRIG_PRESSED),
                        CTX_GAME, "crouch", KEY_A, TRIG_PRESSED));
}

MU_TEST(test_find_action_returns_the_first_of_several_keys) {
    const keybind_handle a = keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "move");
    keybind_set(&table, CTX_GAME, KEY_B, TRIG_PRESSED, "move");

    mu_check(keybind_find_action(&table, CTX_GAME, "move") == a);
}

MU_TEST_SUITE(keybind_suite) {
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
    MU_RUN_TEST(test_same_key_reassigns_in_place);
    MU_RUN_TEST(test_one_action_on_several_keys);
    MU_RUN_TEST(test_swapping_two_keys_works_in_either_order);
    MU_RUN_TEST(test_find_action_returns_the_first_of_several_keys);
    MU_RUN_TEST(test_same_key_different_contexts_both_kept);
    MU_RUN_TEST(test_same_key_different_triggers_both_kept);
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
    MU_RUN_SUITE(keybind_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
