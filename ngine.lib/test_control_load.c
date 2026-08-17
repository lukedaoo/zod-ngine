#include "../thirdparty/minunit.h"

#include <stdio.h>
#include <string.h>

#define LOG_IMPLEMENTATION
#include "log.h"

#define ARRAY_LIST_IMPLEMENTATION
#include "collections/array_list.h"

#define ACTION_IMPLEMENTATION
#include "action.h"

#define SCF_IMPLEMENTATION
#include "scf.h"

#define INI_IMPLEMENTATION
#include "ini.h"

#define CONTROL_IMPLEMENTATION
#include "control.h"

#define CONTROL_LOAD_IMPLEMENTATION
#include "control_load.h"

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

MU_TEST(test_parse_key_with_default_trigger) {
    mu_check(control_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "jump A\n",
                                      &fake_vocab));

    const control_handle h = control_find(&table, CTX_GAME, "jump");
    mu_check(binding_is(h, CTX_GAME, "jump", KEY_A, TRIG_PRESSED));
}

MU_TEST(test_parse_each_explicit_trigger) {
    mu_check(control_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "tap      A pressed\n"
                                      "hold     B down\n"
                                      "let_go   C released\n",
                                      &fake_vocab));

    mu_check(binding_is(control_find(&table, CTX_GAME, "tap"), CTX_GAME, "tap", KEY_A,
                        TRIG_PRESSED));
    mu_check(binding_is(control_find(&table, CTX_GAME, "hold"), CTX_GAME, "hold", KEY_B,
                        TRIG_DOWN));
    mu_check(binding_is(control_find(&table, CTX_GAME, "let_go"), CTX_GAME, "let_go",
                        KEY_C, TRIG_RELEASED));
}

MU_TEST(test_parse_multiple_contexts) {
    mu_check(control_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "jump A\n"
                                      ":/input.menu\n"
                                      "confirm A\n",
                                      &fake_vocab));

    mu_assert_int_eq(2, (int)live_count());
    mu_check(binding_is(control_find(&table, CTX_MENU, "confirm"), CTX_MENU, "confirm",
                        KEY_A, TRIG_PRESSED));
}

MU_TEST(test_parse_ignores_sections_without_prefix) {
    mu_check(control_merge_scf_string(&table,
                                      ":/window\n"
                                      "width 800\n"
                                      ":/input.game\n"
                                      "jump A\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
    mu_check(control_find(&table, CTX_GAME, "jump") != CONTROL_HANDLE_INVALID);
}

MU_TEST(test_parse_skips_prefix_without_context_name) {
    mu_check(control_merge_scf_string(&table,
                                      ":/input.\n"
                                      "jump A\n"
                                      ":/input.game\n"
                                      "crouch B\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
    mu_check(control_find(&table, CTX_GAME, "crouch") != CONTROL_HANDLE_INVALID);
}

MU_TEST(test_parse_skips_unknown_context_but_keeps_the_rest) {
    mu_check(control_merge_scf_string(&table,
                                      ":/input.nowhere\n"
                                      "jump A\n"
                                      ":/input.game\n"
                                      "crouch B\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
    mu_check(control_find(&table, CTX_GAME, "crouch") != CONTROL_HANDLE_INVALID);
}

MU_TEST(test_parse_skips_unknown_key_but_keeps_the_rest) {
    mu_check(control_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "jump   NOPE\n"
                                      "crouch B\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
    mu_check(control_find(&table, CTX_GAME, "jump") == CONTROL_HANDLE_INVALID);
    mu_check(control_find(&table, CTX_GAME, "crouch") != CONTROL_HANDLE_INVALID);
}

MU_TEST(test_parse_skips_unknown_trigger_entirely) {
    mu_check(control_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "jump   A sideways\n"
                                      "crouch B\n",
                                      &fake_vocab));

    mu_check(control_find(&table, CTX_GAME, "jump") == CONTROL_HANDLE_INVALID);
    mu_check(control_find(&table, CTX_GAME, "crouch") != CONTROL_HANDLE_INVALID);
}

MU_TEST(test_parse_skips_oversized_action_name) {
    char text[256];
    char name[CONTROL_ACTION_MAX + 8];
    memset(name, 'x', sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    snprintf(text, sizeof(text), ":/input.game\n%s A\ncrouch B\n", name);

    mu_check(control_merge_scf_string(&table, text, &fake_vocab));
    mu_assert_int_eq(1, (int)live_count());
    mu_check(control_find(&table, CTX_GAME, "crouch") != CONTROL_HANDLE_INVALID);
}

MU_TEST(test_parse_handles_comments_and_extra_whitespace) {
    mu_check(control_merge_scf_string(&table,
                                      "; leading comment\n"
                                      ":/input.game\n"
                                      "jump      A     down    ; trailing comment\n"
                                      "\n"
                                      "crouch\tB\n",
                                      &fake_vocab));

    mu_check(binding_is(control_find(&table, CTX_GAME, "jump"), CTX_GAME, "jump", KEY_A,
                        TRIG_DOWN));
    mu_check(binding_is(control_find(&table, CTX_GAME, "crouch"), CTX_GAME, "crouch",
                        KEY_B, TRIG_PRESSED));
}

MU_TEST(test_parse_later_duplicate_action_wins) {
    mu_check(control_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "jump A\n"
                                      "jump B down\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
    mu_check(binding_is(control_find(&table, CTX_GAME, "jump"), CTX_GAME, "jump", KEY_B,
                        TRIG_DOWN));
}

MU_TEST(test_parse_conflicting_key_keeps_the_first) {
    mu_check(control_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "jump   A\n"
                                      "crouch A\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
    mu_check(control_find(&table, CTX_GAME, "jump") != CONTROL_HANDLE_INVALID);
    mu_check(control_find(&table, CTX_GAME, "crouch") == CONTROL_HANDLE_INVALID);
}

MU_TEST(test_merge_folds_into_existing_bindings) {
    control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);

    mu_check(control_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "crouch B\n",
                                      &fake_vocab));

    mu_assert_int_eq(2, (int)live_count());
    mu_check(control_find(&table, CTX_GAME, "jump") != CONTROL_HANDLE_INVALID);
}

MU_TEST(test_merge_override_wins_and_leaves_others_alone) {
    mu_check(control_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "jump   A\n"
                                      "crouch B\n",
                                      &fake_vocab));
    mu_check(control_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "jump C down\n",
                                      &fake_vocab));

    mu_assert_int_eq(2, (int)live_count());
    mu_check(binding_is(control_find(&table, CTX_GAME, "jump"), CTX_GAME, "jump", KEY_C,
                        TRIG_DOWN));
    mu_check(binding_is(control_find(&table, CTX_GAME, "crouch"), CTX_GAME, "crouch",
                        KEY_B, TRIG_PRESSED));
}

MU_TEST(test_load_of_missing_file_leaves_table_unchanged) {
    control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);

    mu_check(!control_load(&table, "testdata/definitely_not_here.scf", &fake_vocab));
    mu_assert_int_eq(1, (int)live_count());
    mu_check(binding_is(control_find(&table, CTX_GAME, "jump"), CTX_GAME, "jump", KEY_A,
                        TRIG_PRESSED));
}

MU_TEST(test_merge_of_missing_file_leaves_table_unchanged) {
    control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);

    mu_check(!control_merge(&table, "testdata/definitely_not_here.scf", &fake_vocab));
    mu_assert_int_eq(1, (int)live_count());
}

MU_TEST(test_merge_string_null_args_return_false) {
    mu_check(!control_merge_scf_string(NULL, ":/input.game\njump A\n", &fake_vocab));
    mu_check(!control_merge_scf_string(&table, NULL, &fake_vocab));
    mu_check(!control_load(&table, NULL, &fake_vocab));
    mu_check(!control_merge(&table, NULL, &fake_vocab));
}

MU_TEST(test_incomplete_vocab_fails_the_load) {
    const control_vocab no_resolver = {
         .contexts = fake_contexts, .context_count = 2, .key_from_name = NULL};
    const control_vocab no_contexts = {
         .contexts = NULL, .context_count = 0, .key_from_name = fake_key_from_name};

    mu_check(!control_merge_scf_string(&table, ":/input.game\njump A\n", &no_resolver));
    mu_check(!control_merge_scf_string(&table, ":/input.game\njump A\n", &no_contexts));
    mu_check(!control_merge_scf_string(&table, ":/input.game\njump A\n", NULL));
    mu_assert_int_eq(0, (int)control_count(&table));
}

MU_TEST(test_key_to_string_without_resolver_returns_null) {
    const control_vocab no_writer = {.contexts      = fake_contexts,
                                     .context_count = 2,
                                     .key_from_name = fake_key_from_name,
                                     .key_to_name   = NULL};

    mu_check(control_merge_scf_string(&table, ":/input.game\njump A\n", &no_writer));
    mu_check(control_key_to_string(&no_writer, KEY_A) == NULL);
    mu_check(control_key_to_string(NULL, KEY_A) == NULL);
}

MU_TEST(test_custom_trigger_table_overrides_builtin) {
    static const control_trigger_name custom[] = {
         {"tap", 7},
         {"hold", 8},
    };
    const control_vocab custom_vocab = {.contexts      = fake_contexts,
                                        .context_count = 2,
                                        .triggers      = custom,
                                        .trigger_count = 2,
                                        .key_from_name = fake_key_from_name,
                                        .key_to_name   = fake_key_to_name};

    mu_check(control_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "jump   A hold\n"
                                      "crouch B\n"
                                      "shoot  C down\n",
                                      &custom_vocab));

    mu_check(
         binding_is(control_find(&table, CTX_GAME, "jump"), CTX_GAME, "jump", KEY_A, 8));
    mu_check(binding_is(control_find(&table, CTX_GAME, "crouch"), CTX_GAME, "crouch",
                        KEY_B, 7));
    mu_check(control_find(&table, CTX_GAME, "shoot") == CONTROL_HANDLE_INVALID);
}

MU_TEST(test_integration_load_rebind_enumerate) {
    mu_check(control_merge_scf_string(&table,
                                      "; beatup-ish defaults\n"
                                      ":/input.game\n"
                                      "press_top_left     A\n"
                                      "press_bar          Space\n"
                                      ":/input.menu\n"
                                      "confirm            Space\n",
                                      &fake_vocab));
    mu_assert_int_eq(3, (int)live_count());

    const control_handle bar = control_find(&table, CTX_GAME, "press_bar");
    mu_check(control_set(&table, CTX_GAME, "press_bar", KEY_B, TRIG_DOWN) == bar);
    mu_check(binding_is(bar, CTX_GAME, "press_bar", KEY_B, TRIG_DOWN));

    mu_check(control_set(&table, CTX_GAME, "press_bar", KEY_A, TRIG_PRESSED) ==
             CONTROL_HANDLE_INVALID);
    mu_check(binding_is(bar, CTX_GAME, "press_bar", KEY_B, TRIG_DOWN));

    size_t rendered = 0;
    for (size_t i = 0; i < control_count(&table); i++) {
        const control_handle h = control_at(&table, i);
        if (h == CONTROL_HANDLE_INVALID) continue;

        control_binding out;
        mu_check(control_lookup(&table, h, &out));
        mu_check(control_key_to_string(&fake_vocab, out.key) != NULL);
        mu_check(control_trigger_to_string(&fake_vocab, out.trigger) != NULL);
        rendered++;
    }
    mu_assert_int_eq(3, (int)rendered);
}

MU_TEST(test_ini_parses_the_same_bindings_as_scf) {
    mu_check(control_merge_ini_string(&table,
                                      "[input.game]\n"
                                      "jump   = A\n"
                                      "crouch = B down\n"
                                      "[input.menu]\n"
                                      "confirm = Space\n",
                                      &fake_vocab));

    mu_assert_int_eq(3, (int)live_count());
    mu_check(binding_is(control_find(&table, CTX_GAME, "jump"), CTX_GAME, "jump", KEY_A,
                        TRIG_PRESSED));
    mu_check(binding_is(control_find(&table, CTX_GAME, "crouch"), CTX_GAME, "crouch",
                        KEY_B, TRIG_DOWN));
    mu_check(binding_is(control_find(&table, CTX_MENU, "confirm"), CTX_MENU, "confirm",
                        KEY_SPACE, TRIG_PRESSED));
}

MU_TEST(test_ini_ignores_sections_without_prefix) {
    mu_check(control_merge_ini_string(&table,
                                      "[window]\n"
                                      "width = 800\n"
                                      "[input.game]\n"
                                      "jump = A\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
}

MU_TEST(test_ini_skips_bad_lines_and_keeps_the_rest) {
    mu_check(control_merge_ini_string(&table,
                                      "[input.game]\n"
                                      "jump   = NOPE\n"
                                      "duck   = A sideways\n"
                                      "crouch = B\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
    mu_check(control_find(&table, CTX_GAME, "crouch") != CONTROL_HANDLE_INVALID);
}

MU_TEST(test_ini_conflict_rules_match_scf) {
    mu_check(control_merge_ini_string(&table,
                                      "[input.game]\n"
                                      "jump   = A\n"
                                      "crouch = A\n"
                                      "jump   = B down\n",
                                      &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
    mu_check(binding_is(control_find(&table, CTX_GAME, "jump"), CTX_GAME, "jump", KEY_B,
                        TRIG_DOWN));
    mu_check(control_find(&table, CTX_GAME, "crouch") == CONTROL_HANDLE_INVALID);
}

MU_TEST(test_ini_and_scf_compose_in_one_table) {
    mu_check(control_merge_scf_string(&table,
                                      ":/input.game\n"
                                      "jump A\n",
                                      &fake_vocab));
    mu_check(control_merge_ini_string(&table,
                                      "[input.game]\n"
                                      "crouch = B\n",
                                      &fake_vocab));

    mu_assert_int_eq(2, (int)live_count());
}

MU_TEST(test_dispatch_rejects_unknown_extension) {
    mu_check(!control_load(&table, "controls.toml", &fake_vocab));
    mu_check(!control_load(&table, "controls", &fake_vocab));
    mu_check(!control_load(&table, NULL, &fake_vocab));
    mu_check(!control_merge(&table, "controls.toml", &fake_vocab));
    mu_check(!control_merge(&table, NULL, &fake_vocab));
}

MU_TEST(test_dispatch_accepts_known_extensions_and_reports_missing_files) {
    control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);

    mu_check(!control_load(&table, "testdata/definitely_not_here.scf", &fake_vocab));
    mu_check(!control_load(&table, "testdata/definitely_not_here.ini", &fake_vocab));
    mu_check(!control_merge(&table, "testdata/definitely_not_here.ini", &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
}

MU_TEST(test_load_scf_and_ini_of_missing_file_leave_table_unchanged) {
    control_set(&table, CTX_GAME, "jump", KEY_A, TRIG_PRESSED);

    mu_check(!control_load_scf(&table, "testdata/definitely_not_here.scf", &fake_vocab));
    mu_check(!control_load_ini(&table, "testdata/definitely_not_here.ini", &fake_vocab));
    mu_check(!control_merge_scf(&table, "testdata/definitely_not_here.scf", &fake_vocab));
    mu_check(!control_merge_ini(&table, "testdata/definitely_not_here.ini", &fake_vocab));

    mu_assert_int_eq(1, (int)live_count());
    mu_check(binding_is(control_find(&table, CTX_GAME, "jump"), CTX_GAME, "jump", KEY_A,
                        TRIG_PRESSED));
}

MU_TEST(test_ini_string_incomplete_vocab_fails) {
    const control_vocab no_resolver = {
         .contexts = fake_contexts, .context_count = 2, .key_from_name = NULL};

    mu_check(!control_merge_ini_string(&table, "[input.game]\njump = A\n", &no_resolver));
    mu_check(!control_merge_ini_string(&table, NULL, &fake_vocab));
    mu_assert_int_eq(0, (int)control_count(&table));
}

MU_TEST_SUITE(control_load_suite) {
    MU_SUITE_CONFIGURE(&setup, &teardown);

    MU_RUN_TEST(test_parse_key_with_default_trigger);
    MU_RUN_TEST(test_parse_each_explicit_trigger);
    MU_RUN_TEST(test_parse_multiple_contexts);
    MU_RUN_TEST(test_parse_ignores_sections_without_prefix);
    MU_RUN_TEST(test_parse_skips_prefix_without_context_name);
    MU_RUN_TEST(test_parse_skips_unknown_context_but_keeps_the_rest);
    MU_RUN_TEST(test_parse_skips_unknown_key_but_keeps_the_rest);
    MU_RUN_TEST(test_parse_skips_unknown_trigger_entirely);
    MU_RUN_TEST(test_parse_skips_oversized_action_name);
    MU_RUN_TEST(test_parse_handles_comments_and_extra_whitespace);
    MU_RUN_TEST(test_parse_later_duplicate_action_wins);
    MU_RUN_TEST(test_parse_conflicting_key_keeps_the_first);
    MU_RUN_TEST(test_merge_folds_into_existing_bindings);
    MU_RUN_TEST(test_merge_override_wins_and_leaves_others_alone);
    MU_RUN_TEST(test_load_of_missing_file_leaves_table_unchanged);
    MU_RUN_TEST(test_merge_of_missing_file_leaves_table_unchanged);
    MU_RUN_TEST(test_merge_string_null_args_return_false);
    MU_RUN_TEST(test_incomplete_vocab_fails_the_load);
    MU_RUN_TEST(test_key_to_string_without_resolver_returns_null);
    MU_RUN_TEST(test_custom_trigger_table_overrides_builtin);
    MU_RUN_TEST(test_integration_load_rebind_enumerate);

    MU_RUN_TEST(test_ini_parses_the_same_bindings_as_scf);
    MU_RUN_TEST(test_ini_ignores_sections_without_prefix);
    MU_RUN_TEST(test_ini_skips_bad_lines_and_keeps_the_rest);
    MU_RUN_TEST(test_ini_conflict_rules_match_scf);
    MU_RUN_TEST(test_ini_and_scf_compose_in_one_table);
    MU_RUN_TEST(test_dispatch_rejects_unknown_extension);
    MU_RUN_TEST(test_dispatch_accepts_known_extensions_and_reports_missing_files);
    MU_RUN_TEST(test_load_scf_and_ini_of_missing_file_leave_table_unchanged);
    MU_RUN_TEST(test_ini_string_incomplete_vocab_fails);
}

int main(void) {
    MU_RUN_SUITE(control_load_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
