#include "../thirdparty/minunit.h"

#include <stdio.h>
#include <string.h>

#define LOG_IMPLEMENTATION
#include "log.h"

#define ARRAY_LIST_IMPLEMENTATION
#include "collections/array_list.h"

#define SCF_IMPLEMENTATION
#include "scf.h"

#define INI_IMPLEMENTATION
#include "ini.h"

#define KEYBIND_IMPLEMENTATION
#include "keybind.h"

#define KEYBIND_LOAD_IMPLEMENTATION
#include "keybind_load.h"

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

static bool bound_to(const int context, const int key, const int trigger,
                     const char *action) {
    keybind out;
    if (!keybind_lookup(&table, keybind_find_by_key(&table, context, key, trigger), &out))
        return false;
    return strcmp(out.action, action) == 0;
}

static size_t live_count(void) {
    size_t live = 0;
    for (size_t i = 0; i < keybind_count(&table); i++)
        if (keybind_at(&table, i) != KEYBIND_HANDLE_INVALID) live++;
    return live;
}

MU_TEST(test_parse_key_trigger_action) {
    mu_check(keybind_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "A pressed jump\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
    mu_check(bound_to(CTX_GAME, KEY_A, TRIG_PRESSED, "jump"));
}

MU_TEST(test_parse_each_explicit_trigger) {
    mu_check(keybind_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "A pressed  tap\n"
                                      "B down     hold\n"
                                      "C released let_go\n",
                                      &fake_vocab));

    mu_check(bound_to(CTX_GAME, KEY_A, TRIG_PRESSED, "tap"));
    mu_check(bound_to(CTX_GAME, KEY_B, TRIG_DOWN, "hold"));
    mu_check(bound_to(CTX_GAME, KEY_C, TRIG_RELEASED, "let_go"));
}

MU_TEST(test_parse_multiple_contexts) {
    mu_check(keybind_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "A pressed jump\n"
                                      ":/input.menu\n"
                                      "A pressed confirm\n",
                                      &fake_vocab));

    mu_assert_int_eq(2, (int)live_count());
    mu_check(bound_to(CTX_GAME, KEY_A, TRIG_PRESSED, "jump"));
    mu_check(bound_to(CTX_MENU, KEY_A, TRIG_PRESSED, "confirm"));
}

MU_TEST(test_parse_one_action_on_several_keys) {
    mu_check(keybind_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "A pressed move_left\n"
                                      "B pressed move_left\n",
                                      &fake_vocab));

    mu_assert_int_eq(2, (int)live_count());
    mu_check(bound_to(CTX_GAME, KEY_A, TRIG_PRESSED, "move_left"));
    mu_check(bound_to(CTX_GAME, KEY_B, TRIG_PRESSED, "move_left"));
}

MU_TEST(test_parse_swap_is_expressible) {
    mu_check(keybind_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "A pressed jump\n"
                                      "B pressed crouch\n",
                                      &fake_vocab));
    mu_check(keybind_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "B pressed jump\n"
                                      "A pressed crouch\n",
                                      &fake_vocab));

    mu_assert_int_eq(2, (int)live_count());
    mu_check(bound_to(CTX_GAME, KEY_B, TRIG_PRESSED, "jump"));
    mu_check(bound_to(CTX_GAME, KEY_A, TRIG_PRESSED, "crouch"));
}

MU_TEST(test_parse_ignores_sections_without_prefix) {
    mu_check(keybind_merge_scf_string(&table,
                                      ":/window\n"
                                      "width 800\n"
                                      ":/input.game\n"
                                      "A pressed jump\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
}

MU_TEST(test_parse_skips_prefix_without_context_name) {
    mu_check(keybind_merge_scf_string(&table,
                                      ":/input.\n"
                                      "A pressed jump\n"
                                      ":/input.game\n"
                                      "B pressed crouch\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
    mu_check(bound_to(CTX_GAME, KEY_B, TRIG_PRESSED, "crouch"));
}

MU_TEST(test_parse_skips_unknown_context_but_keeps_the_rest) {
    mu_check(keybind_merge_scf_string(&table,
                                      ":/input.nowhere\n"
                                      "A pressed jump\n"
                                      ":/input.game\n"
                                      "B pressed crouch\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
    mu_check(bound_to(CTX_GAME, KEY_B, TRIG_PRESSED, "crouch"));
}

MU_TEST(test_parse_skips_unknown_key_but_keeps_the_rest) {
    mu_check(keybind_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "NOPE pressed jump\n"
                                      "B    pressed crouch\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
    mu_check(keybind_find_action(&table, CTX_GAME, "jump") == KEYBIND_HANDLE_INVALID);
    mu_check(bound_to(CTX_GAME, KEY_B, TRIG_PRESSED, "crouch"));
}

MU_TEST(test_parse_skips_unknown_trigger_entirely) {
    mu_check(keybind_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "A sideways jump\n"
                                      "B pressed  crouch\n",
                                      &fake_vocab));

    mu_check(keybind_find_action(&table, CTX_GAME, "jump") == KEYBIND_HANDLE_INVALID);
    mu_check(bound_to(CTX_GAME, KEY_B, TRIG_PRESSED, "crouch"));
}

MU_TEST(test_parse_skips_oversized_action_name) {
    char text[256];
    char action[KEYBIND_ACTION_MAX + 8];
    memset(action, 'x', sizeof(action) - 1);
    action[sizeof(action) - 1] = '\0';
    snprintf(text, sizeof(text), ":/input.game\nA pressed %s\nB pressed crouch\n",
             action);

    mu_check(keybind_merge_scf_string(&table, text, &fake_vocab));
    mu_assert_int_eq(1, (int)live_count());
    mu_check(bound_to(CTX_GAME, KEY_B, TRIG_PRESSED, "crouch"));
}

MU_TEST(test_parse_handles_comments_and_extra_whitespace) {
    mu_check(keybind_merge_scf_string(&table,
                                      "; leading comment\n"
                                      ":/input.game\n"
                                      "A      down     jump    ; trailing comment\n"
                                      "\n"
                                      "B\tpressed\tcrouch\n",
                                      &fake_vocab));

    mu_check(bound_to(CTX_GAME, KEY_A, TRIG_DOWN, "jump"));
    mu_check(bound_to(CTX_GAME, KEY_B, TRIG_PRESSED, "crouch"));
}

MU_TEST(test_parse_repeated_key_last_wins) {
    mu_check(keybind_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "A pressed jump\n"
                                      "A pressed crouch\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
    mu_check(bound_to(CTX_GAME, KEY_A, TRIG_PRESSED, "crouch"));
}

MU_TEST(test_parse_same_key_different_triggers_both_kept) {
    mu_check(keybind_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "A pressed tap\n"
                                      "A down    hold\n",
                                      &fake_vocab));

    mu_assert_int_eq(2, (int)live_count());
    mu_check(bound_to(CTX_GAME, KEY_A, TRIG_PRESSED, "tap"));
    mu_check(bound_to(CTX_GAME, KEY_A, TRIG_DOWN, "hold"));
}

MU_TEST(test_merge_folds_into_existing_bindings) {
    keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");

    mu_check(keybind_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "B pressed crouch\n",
                                      &fake_vocab));

    mu_assert_int_eq(2, (int)live_count());
    mu_check(bound_to(CTX_GAME, KEY_A, TRIG_PRESSED, "jump"));
}

MU_TEST(test_merge_override_reassigns_and_leaves_others_alone) {
    mu_check(keybind_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "A pressed jump\n"
                                      "B pressed crouch\n",
                                      &fake_vocab));
    mu_check(keybind_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "A pressed shoot\n",
                                      &fake_vocab));

    mu_assert_int_eq(2, (int)live_count());
    mu_check(bound_to(CTX_GAME, KEY_A, TRIG_PRESSED, "shoot"));
    mu_check(bound_to(CTX_GAME, KEY_B, TRIG_PRESSED, "crouch"));
}

MU_TEST(test_load_of_missing_file_leaves_table_unchanged) {
    keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");

    mu_check(!keybind_load(&table, "testdata/definitely_not_here.scf", &fake_vocab));
    mu_assert_int_eq(1, (int)live_count());
    mu_check(bound_to(CTX_GAME, KEY_A, TRIG_PRESSED, "jump"));
}

MU_TEST(test_merge_of_missing_file_leaves_table_unchanged) {
    keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");

    mu_check(!keybind_merge(&table, "testdata/definitely_not_here.ini", &fake_vocab));
    mu_assert_int_eq(1, (int)live_count());
}

MU_TEST(test_merge_string_null_args_return_false) {
    mu_check(
         !keybind_merge_scf_string(NULL, ":/input.game\nA pressed jump\n", &fake_vocab));
    mu_check(!keybind_merge_scf_string(&table, NULL, &fake_vocab));
    mu_check(!keybind_load(&table, NULL, &fake_vocab));
    mu_check(!keybind_merge(&table, NULL, &fake_vocab));
}

MU_TEST(test_incomplete_vocab_fails_the_load) {
    const keybind_vocab no_resolver = {
         .contexts = fake_contexts, .context_count = 2, .key_from_name = NULL};
    const keybind_vocab no_contexts = {
         .contexts = NULL, .context_count = 0, .key_from_name = fake_key_from_name};

    mu_check(!keybind_merge_scf_string(&table, ":/input.game\nA pressed jump\n",
                                       &no_resolver));
    mu_check(!keybind_merge_scf_string(&table, ":/input.game\nA pressed jump\n",
                                       &no_contexts));
    mu_check(!keybind_merge_scf_string(&table, ":/input.game\nA pressed jump\n", NULL));
    mu_assert_int_eq(0, (int)keybind_count(&table));
}

MU_TEST(test_custom_trigger_table_overrides_builtin) {
    static const keybind_trigger_name custom[] = {
         {"tap", 7},
         {"hold", 8},
    };
    const keybind_vocab custom_vocab = {.contexts      = fake_contexts,
                                        .context_count = 2,
                                        .triggers      = custom,
                                        .trigger_count = 2,
                                        .key_from_name = fake_key_from_name,
                                        .key_to_name   = fake_key_to_name};

    mu_check(keybind_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "A hold jump\n"
                                      "B tap  crouch\n"
                                      "C down shoot\n",
                                      &custom_vocab));

    mu_check(bound_to(CTX_GAME, KEY_A, 8, "jump"));
    mu_check(bound_to(CTX_GAME, KEY_B, 7, "crouch"));
    mu_check(keybind_find_action(&table, CTX_GAME, "shoot") == KEYBIND_HANDLE_INVALID);
}

MU_TEST(test_ini_parses_the_same_bindings_as_scf) {
    mu_check(keybind_merge_ini_string(&table,
                                      "[input.game]\n"
                                      "A = pressed jump\n"
                                      "B = down crouch\n"
                                      "[input.menu]\n"
                                      "Space = pressed confirm\n",
                                      &fake_vocab));

    mu_assert_int_eq(3, (int)live_count());
    mu_check(bound_to(CTX_GAME, KEY_A, TRIG_PRESSED, "jump"));
    mu_check(bound_to(CTX_GAME, KEY_B, TRIG_DOWN, "crouch"));
    mu_check(bound_to(CTX_MENU, KEY_SPACE, TRIG_PRESSED, "confirm"));
}

MU_TEST(test_ini_ignores_sections_without_prefix) {
    mu_check(keybind_merge_ini_string(&table,
                                      "[window]\n"
                                      "width = 800\n"
                                      "[input.game]\n"
                                      "A = pressed jump\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
}

MU_TEST(test_ini_skips_bad_lines_and_keeps_the_rest) {
    mu_check(keybind_merge_ini_string(&table,
                                      "[input.game]\n"
                                      "NOPE = pressed jump\n"
                                      "A = sideways duck\n"
                                      "B = pressed crouch\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
    mu_check(bound_to(CTX_GAME, KEY_B, TRIG_PRESSED, "crouch"));
}

MU_TEST(test_ini_repeated_key_last_wins) {
    mu_check(keybind_merge_ini_string(&table,
                                      "[input.game]\n"
                                      "A = pressed jump\n"
                                      "A = pressed crouch\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
    mu_check(bound_to(CTX_GAME, KEY_A, TRIG_PRESSED, "crouch"));
}

MU_TEST(test_ini_and_scf_compose_in_one_table) {
    mu_check(keybind_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "A pressed jump\n",
                                      &fake_vocab));
    mu_check(keybind_merge_ini_string(&table,
                                      "[input.game]\n"
                                      "B = pressed crouch\n",
                                      &fake_vocab));

    mu_assert_int_eq(2, (int)live_count());
}

MU_TEST(test_dispatch_rejects_unknown_extension) {
    mu_check(!keybind_load(&table, "keybind.toml", &fake_vocab));
    mu_check(!keybind_load(&table, "keybind", &fake_vocab));
    mu_check(!keybind_merge(&table, "keybind.toml", &fake_vocab));
}

MU_TEST(test_dispatch_accepts_known_extensions_and_reports_missing_files) {
    keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");

    mu_check(!keybind_load(&table, "testdata/definitely_not_here.scf", &fake_vocab));
    mu_check(!keybind_load(&table, "testdata/definitely_not_here.ini", &fake_vocab));
    mu_check(!keybind_merge(&table, "testdata/definitely_not_here.ini", &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
}

MU_TEST(test_explicit_per_format_entry_points_on_missing_files) {
    keybind_set(&table, CTX_GAME, KEY_A, TRIG_PRESSED, "jump");

    mu_check(!keybind_load_scf(&table, "testdata/definitely_not_here.scf", &fake_vocab));
    mu_check(!keybind_load_ini(&table, "testdata/definitely_not_here.ini", &fake_vocab));
    mu_check(!keybind_merge_scf(&table, "testdata/definitely_not_here.scf", &fake_vocab));
    mu_check(!keybind_merge_ini(&table, "testdata/definitely_not_here.ini", &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
    mu_check(bound_to(CTX_GAME, KEY_A, TRIG_PRESSED, "jump"));
}

MU_TEST(test_integration_load_rebind_enumerate) {
    mu_check(keybind_merge_scf_string(&table,
                                      "; beatup-ish defaults\n"
                                      ":/input.game\n"
                                      "A     pressed press_top_left\n"
                                      "Space pressed press_bar\n"
                                      ":/input.menu\n"
                                      "Space pressed confirm\n",
                                      &fake_vocab));
    mu_assert_int_eq(3, (int)live_count());

    const keybind_handle bar = keybind_find_action(&table, CTX_GAME, "press_bar");
    mu_check(keybind_set(&table, CTX_GAME, KEY_B, TRIG_DOWN, "press_bar") !=
             KEYBIND_HANDLE_INVALID);
    mu_check(bound_to(CTX_GAME, KEY_B, TRIG_DOWN, "press_bar"));

    // the old row survives: an action may hold several keys
    mu_check(keybind_lookup(&table, bar, NULL) == false);
    mu_assert_int_eq(4, (int)live_count());

    size_t rendered = 0;
    for (size_t i = 0; i < keybind_count(&table); i++) {
        const keybind_handle h = keybind_at(&table, i);
        if (h == KEYBIND_HANDLE_INVALID) continue;

        keybind out;
        mu_check(keybind_lookup(&table, h, &out));
        mu_check(keybind_key_to_string(&fake_vocab, out.key) != NULL);
        mu_check(keybind_trigger_to_string(&fake_vocab, out.trigger) != NULL);
        rendered++;
    }
    mu_assert_int_eq(4, (int)rendered);
}

MU_TEST_SUITE(keybind_load_suite) {
    MU_SUITE_CONFIGURE(&setup, &teardown);

    MU_RUN_TEST(test_parse_key_trigger_action);
    MU_RUN_TEST(test_parse_each_explicit_trigger);
    MU_RUN_TEST(test_parse_multiple_contexts);
    MU_RUN_TEST(test_parse_one_action_on_several_keys);
    MU_RUN_TEST(test_parse_swap_is_expressible);
    MU_RUN_TEST(test_parse_ignores_sections_without_prefix);
    MU_RUN_TEST(test_parse_skips_prefix_without_context_name);
    MU_RUN_TEST(test_parse_skips_unknown_context_but_keeps_the_rest);
    MU_RUN_TEST(test_parse_skips_unknown_key_but_keeps_the_rest);
    MU_RUN_TEST(test_parse_skips_unknown_trigger_entirely);
    MU_RUN_TEST(test_parse_skips_oversized_action_name);
    MU_RUN_TEST(test_parse_handles_comments_and_extra_whitespace);
    MU_RUN_TEST(test_parse_repeated_key_last_wins);
    MU_RUN_TEST(test_parse_same_key_different_triggers_both_kept);

    MU_RUN_TEST(test_merge_folds_into_existing_bindings);
    MU_RUN_TEST(test_merge_override_reassigns_and_leaves_others_alone);
    MU_RUN_TEST(test_load_of_missing_file_leaves_table_unchanged);
    MU_RUN_TEST(test_merge_of_missing_file_leaves_table_unchanged);
    MU_RUN_TEST(test_merge_string_null_args_return_false);
    MU_RUN_TEST(test_incomplete_vocab_fails_the_load);
    MU_RUN_TEST(test_custom_trigger_table_overrides_builtin);

    MU_RUN_TEST(test_ini_parses_the_same_bindings_as_scf);
    MU_RUN_TEST(test_ini_ignores_sections_without_prefix);
    MU_RUN_TEST(test_ini_skips_bad_lines_and_keeps_the_rest);
    MU_RUN_TEST(test_ini_repeated_key_last_wins);
    MU_RUN_TEST(test_ini_and_scf_compose_in_one_table);

    MU_RUN_TEST(test_dispatch_rejects_unknown_extension);
    MU_RUN_TEST(test_dispatch_accepts_known_extensions_and_reports_missing_files);
    MU_RUN_TEST(test_explicit_per_format_entry_points_on_missing_files);

    MU_RUN_TEST(test_integration_load_rebind_enumerate);
}

int main(void) {
    MU_RUN_SUITE(keybind_load_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
