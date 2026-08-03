#include "../thirdparty/minunit.h"

#include <string.h>

#define ARRAY_LIST_IMPLEMENTATION
#define ACTION_IMPLEMENTATION
#include "action.h"

enum { CTX_GAME = 1, CTX_MENU = 2 };
enum { TYPE_KEYDOWN = 1, TYPE_KEYUP = 2 };

typedef struct {
    int                   call_count;
    action_handle         last_self;
    void                 *last_userdata;
    action_execute_result result;
} spy_record;

static action_execute_result spy_execute(const action_table *table, action_handle self,
                                         void *userdata) {
    (void)table;
    spy_record *rec = (spy_record *)userdata;
    rec->call_count++;
    rec->last_self     = self;
    rec->last_userdata = userdata;
    return rec->result;
}

static spy_record spy_record_new(void) {
    return (spy_record){
         .call_count    = 0,
         .last_self     = ACTION_HANDLE_INVALID,
         .last_userdata = NULL,
         .result        = {.type = ACTION_EXECUTE_RESULT_VOID}  //
    };
}

static spy_record spy_record_with_result(action_execute_result result) {
    return (spy_record){
         .call_count    = 0,
         .last_self     = ACTION_HANDLE_INVALID,
         .last_userdata = NULL,
         .result        = result  //
    };
}

static action_executor spy_executor_new(void) {
    return (action_executor){.execute = spy_execute};
}

MU_TEST(test_init_creates_empty_table) {
    action_table table;
    action_init(&table);
    mu_assert_int_eq(0, (int)table.actions.header.size);
    action_destroy(&table);
}

MU_TEST(test_init_null_table_safe) { action_init(NULL); }

MU_TEST(test_destroy_null_table_safe) { action_destroy(NULL); }

MU_TEST(test_bind_adds_action) {
    action_table table;
    action_init(&table);

    action_executor exec = spy_executor_new();
    mu_check(action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec) !=
             ACTION_HANDLE_INVALID);
    mu_assert_int_eq(1, (int)table.actions.header.size);

    action_destroy(&table);
}

MU_TEST(test_bind_rejects_duplicate_key) {
    action_table table;
    action_init(&table);

    action_executor exec = spy_executor_new();
    mu_check(action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec) !=
             ACTION_HANDLE_INVALID);
    mu_check(action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "duck", &exec) ==
             ACTION_HANDLE_INVALID);
    mu_assert_int_eq(1, (int)table.actions.header.size);

    action_destroy(&table);
}

MU_TEST(test_bind_allows_same_key_different_context) {
    action_table table;
    action_init(&table);

    action_executor exec = spy_executor_new();
    mu_check(action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec) !=
             ACTION_HANDLE_INVALID);
    mu_check(action_bind(&table, CTX_MENU, TYPE_KEYDOWN, 'A', "select", &exec) !=
             ACTION_HANDLE_INVALID);
    mu_assert_int_eq(2, (int)table.actions.header.size);

    action_destroy(&table);
}

MU_TEST(test_bind_rejects_name_too_long) {
    action_table table;
    action_init(&table);

    action_executor exec = spy_executor_new();
    char            long_name[ACTION_NAME_MAX + 1];
    memset(long_name, 'x', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';

    mu_check(action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', long_name, &exec) ==
             ACTION_HANDLE_INVALID);
    mu_assert_int_eq(0, (int)table.actions.header.size);

    action_destroy(&table);
}

MU_TEST(test_bind_null_table_returns_false) {
    action_executor exec = spy_executor_new();
    mu_check(action_bind(NULL, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec) ==
             ACTION_HANDLE_INVALID);
}

MU_TEST(test_bind_null_executor_returns_false) {
    action_table table;
    action_init(&table);

    mu_check(action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", NULL) ==
             ACTION_HANDLE_INVALID);
    mu_assert_int_eq(0, (int)table.actions.header.size);

    action_destroy(&table);
}

MU_TEST(test_resolve_by_name_finds_match) {
    action_table table;
    action_init(&table);

    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);

    mu_check(action_resolve_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "jump") !=
             ACTION_HANDLE_INVALID);

    action_destroy(&table);
}

MU_TEST(test_resolve_by_name_no_match_returns_null) {
    action_table table;
    action_init(&table);

    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);

    mu_check(action_resolve_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "duck") ==
             ACTION_HANDLE_INVALID);

    action_destroy(&table);
}

MU_TEST(test_resolve_by_name_null_table_returns_null) {
    mu_check(action_resolve_by_name(NULL, CTX_GAME, TYPE_KEYDOWN, "jump") ==
             ACTION_HANDLE_INVALID);
}

MU_TEST(test_resolve_by_key_finds_match) {
    action_table table;
    action_init(&table);

    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);

    mu_check(action_resolve_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'A') !=
             ACTION_HANDLE_INVALID);

    action_destroy(&table);
}

MU_TEST(test_resolve_by_key_no_match_returns_null) {
    action_table table;
    action_init(&table);

    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);

    mu_check(action_resolve_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'B') ==
             ACTION_HANDLE_INVALID);

    action_destroy(&table);
}

MU_TEST(test_resolve_by_key_null_table_returns_null) {
    mu_check(action_resolve_by_key(NULL, CTX_GAME, TYPE_KEYDOWN, 'A') ==
             ACTION_HANDLE_INVALID);
}

MU_TEST(test_rebind_changes_key) {
    action_table table;
    action_init(&table);

    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);

    mu_check(action_rebind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', 'B'));
    mu_check(action_resolve_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'A') ==
             ACTION_HANDLE_INVALID);
    mu_check(action_resolve_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'B') !=
             ACTION_HANDLE_INVALID);

    action_destroy(&table);
}

MU_TEST(test_rebind_no_match_returns_false) {
    action_table table;
    action_init(&table);

    mu_check(!action_rebind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', 'B'));

    action_destroy(&table);
}

MU_TEST(test_rebind_null_table_returns_false) {
    mu_check(!action_rebind(NULL, CTX_GAME, TYPE_KEYDOWN, 'A', 'B'));
}

MU_TEST(test_unbind_by_key_removes_entry) {
    action_table table;
    action_init(&table);

    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);

    mu_check(action_unbind_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'A'));
    mu_assert_int_eq(0, (int)table.actions.header.size);

    action_destroy(&table);
}

MU_TEST(test_unbind_by_key_no_match_returns_false) {
    action_table table;
    action_init(&table);

    mu_check(!action_unbind_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'A'));

    action_destroy(&table);
}

MU_TEST(test_unbind_by_key_leaves_others_intact) {
    action_table table;
    action_init(&table);

    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'B', "duck", &exec);

    mu_check(action_unbind_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'A'));
    mu_assert_int_eq(1, (int)table.actions.header.size);
    mu_check(action_resolve_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'B') !=
             ACTION_HANDLE_INVALID);

    action_destroy(&table);
}

MU_TEST(test_unbind_by_key_null_table_returns_false) {
    mu_check(!action_unbind_by_key(NULL, CTX_GAME, TYPE_KEYDOWN, 'A'));
}

MU_TEST(test_unbind_by_name_removes_entry) {
    action_table table;
    action_init(&table);

    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);

    mu_check(action_unbind_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "jump"));
    mu_assert_int_eq(0, (int)table.actions.header.size);

    action_destroy(&table);
}

MU_TEST(test_unbind_by_name_no_match_returns_false) {
    action_table table;
    action_init(&table);

    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);

    mu_check(!action_unbind_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "duck"));
    mu_assert_int_eq(1, (int)table.actions.header.size);

    action_destroy(&table);
}

MU_TEST(test_unbind_by_name_leaves_others_intact) {
    action_table table;
    action_init(&table);

    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'B', "duck", &exec);

    mu_check(action_unbind_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "jump"));
    mu_assert_int_eq(1, (int)table.actions.header.size);
    mu_check(action_resolve_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "duck") !=
             ACTION_HANDLE_INVALID);

    action_destroy(&table);
}

MU_TEST(test_unbind_by_name_null_table_returns_false) {
    mu_check(!action_unbind_by_name(NULL, CTX_GAME, TYPE_KEYDOWN, "jump"));
}

MU_TEST(test_unbind_all_removes_all_matching) {
    action_table table;
    action_init(&table);

    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'B', "duck", &exec);
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'C', "run", &exec);

    mu_check(action_unbind_all(&table, CTX_GAME, TYPE_KEYDOWN));
    mu_assert_int_eq(0, (int)table.actions.header.size);

    action_destroy(&table);
}

MU_TEST(test_unbind_all_filters_by_context_and_type) {
    action_table table;
    action_init(&table);

    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    action_bind(&table, CTX_GAME, TYPE_KEYUP, 'A', "land", &exec);
    action_bind(&table, CTX_MENU, TYPE_KEYDOWN, 'A', "select", &exec);

    mu_check(action_unbind_all(&table, CTX_GAME, TYPE_KEYDOWN));
    mu_assert_int_eq(2, (int)table.actions.header.size);
    mu_check(action_resolve_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "jump") ==
             ACTION_HANDLE_INVALID);
    mu_check(action_resolve_by_name(&table, CTX_GAME, TYPE_KEYUP, "land") !=
             ACTION_HANDLE_INVALID);
    mu_check(action_resolve_by_name(&table, CTX_MENU, TYPE_KEYDOWN, "select") !=
             ACTION_HANDLE_INVALID);

    action_destroy(&table);
}

MU_TEST(test_unbind_all_no_match_returns_false) {
    action_table table;
    action_init(&table);

    mu_check(!action_unbind_all(&table, CTX_GAME, TYPE_KEYDOWN));

    action_destroy(&table);
}

MU_TEST(test_unbind_all_null_table_returns_false) {
    mu_check(!action_unbind_all(NULL, CTX_GAME, TYPE_KEYDOWN));
}

MU_TEST(test_execute_null_action_returns_void_result) {
    action_execute_result res = action_execute(NULL, ACTION_HANDLE_INVALID, NULL);
    mu_check(res.type == ACTION_EXECUTE_RESULT_VOID);
}

MU_TEST(test_execute_calls_executor_with_action_and_userdata) {
    action_table table;
    action_init(&table);

    spy_record      rec  = spy_record_new();
    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);

    action_handle bound = action_resolve_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'A');
    action_execute(&table, bound, &rec);

    mu_assert_int_eq(1, rec.call_count);
    mu_check(rec.last_self == bound);
    mu_check(rec.last_userdata == &rec);

    action_destroy(&table);
}

MU_TEST(test_execute_returns_executor_result) {
    action_table table;
    action_init(&table);

    action_execute_result want = {.type = ACTION_EXECUTE_RESULT_INT, .value.i = 7};
    spy_record            rec  = spy_record_with_result(want);
    action_executor       exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);

    action_handle          bound = action_resolve_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'A');
    action_execute_result got   = action_execute(&table, bound, &rec);

    mu_check(got.type == ACTION_EXECUTE_RESULT_INT);
    mu_assert_int_eq(7, got.value.i);

    action_destroy(&table);
}

MU_TEST(test_execute_by_name_finds_and_calls) {
    action_table table;
    action_init(&table);

    spy_record      rec  = spy_record_new();
    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);

    action_execute_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "jump", &rec);

    mu_assert_int_eq(1, rec.call_count);
    mu_check(rec.last_userdata == &rec);

    action_destroy(&table);
}

MU_TEST(test_execute_by_name_no_match_returns_void_result) {
    action_table table;
    action_init(&table);

    action_execute_result res =
         action_execute_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "jump", NULL);
    mu_check(res.type == ACTION_EXECUTE_RESULT_VOID);

    action_destroy(&table);
}

MU_TEST(test_execute_by_name_null_table_returns_void_result) {
    action_execute_result res =
         action_execute_by_name(NULL, CTX_GAME, TYPE_KEYDOWN, "jump", NULL);
    mu_check(res.type == ACTION_EXECUTE_RESULT_VOID);
}

MU_TEST(test_integration_multiple_actions_resolve_and_execute_correct_target) {
    action_table table;
    action_init(&table);

    spy_record      rec_jump = spy_record_new();
    spy_record      rec_duck = spy_record_new();
    action_executor exec     = spy_executor_new();

    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'B', "duck", &exec);

    action_execute_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "duck", &rec_duck);

    mu_assert_int_eq(0, rec_jump.call_count);
    mu_assert_int_eq(1, rec_duck.call_count);

    action_handle duck_by_name = action_resolve_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "duck");
    action_handle duck_by_key  = action_resolve_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'B');
    mu_check(duck_by_name == duck_by_key);

    action_destroy(&table);
}

MU_TEST(test_integration_full_flow) {
    action_table table;
    action_init(&table);
    mu_assert_int_eq(0, (int)table.actions.header.size);

    spy_record      rec  = spy_record_new();
    action_executor exec = spy_executor_new();
    mu_check(action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec) !=
             ACTION_HANDLE_INVALID);

    action_handle bound = action_resolve_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "jump");
    mu_check(bound == action_resolve_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'A'));

    action_execute_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "jump", &rec);
    mu_assert_int_eq(1, rec.call_count);
    mu_check(rec.last_self == bound);

    mu_check(action_rebind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', 'B'));
    mu_check(action_resolve_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'A') ==
             ACTION_HANDLE_INVALID);
    mu_check(action_resolve_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'B') == bound);

    action_execute_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "jump", &rec);
    mu_assert_int_eq(2, rec.call_count);

    mu_check(action_unbind_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "jump"));
    mu_assert_int_eq(0, (int)table.actions.header.size);

    action_execute_result res =
         action_execute_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "jump", &rec);
    mu_check(res.type == ACTION_EXECUTE_RESULT_VOID);
    mu_assert_int_eq(2, rec.call_count);

    action_destroy(&table);
}

MU_TEST(test_handle_bind_returns_valid_handle) {
    action_table table;
    action_init(&table);
    action_executor exec = spy_executor_new();
    action_handle   h =
         action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    mu_check(h != ACTION_HANDLE_INVALID);
    action_destroy(&table);
}

MU_TEST(test_handle_bind_duplicate_key_returns_invalid) {
    action_table table;
    action_init(&table);
    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    mu_check(action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "duck", &exec) ==
             ACTION_HANDLE_INVALID);
    action_destroy(&table);
}

MU_TEST(test_handle_resolve_by_name_and_key_agree) {
    action_table table;
    action_init(&table);
    action_executor exec  = spy_executor_new();
    action_handle   bound = action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    mu_check(action_resolve_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "jump") == bound);
    mu_check(action_resolve_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'A') == bound);
    action_destroy(&table);
}

MU_TEST(test_handle_resolve_missing_returns_invalid) {
    action_table table;
    action_init(&table);
    mu_check(action_resolve_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "nope") ==
             ACTION_HANDLE_INVALID);
    mu_check(action_resolve_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'Z') ==
             ACTION_HANDLE_INVALID);
    action_destroy(&table);
}

MU_TEST(test_handle_execute_passes_self_back) {
    action_table table;
    action_init(&table);
    action_executor exec  = spy_executor_new();
    spy_record      rec   = spy_record_new();
    action_handle   bound = action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    action_execute(&table, bound, &rec);
    mu_assert_int_eq(1, rec.call_count);
    mu_check(rec.last_self == bound);
    action_destroy(&table);
}

MU_TEST(test_handle_execute_zero_is_void) {
    action_table table;
    action_init(&table);
    action_executor exec = spy_executor_new();
    spy_record      rec  = spy_record_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    action_execute_result res = action_execute(&table, ACTION_HANDLE_INVALID, &rec);
    mu_assert_int_eq(ACTION_EXECUTE_RESULT_VOID, res.type);
    mu_assert_int_eq(0, rec.call_count);
    action_destroy(&table);
}

MU_TEST(test_handle_execute_out_of_range_is_void) {
    action_table table;
    action_init(&table);
    action_executor exec = spy_executor_new();
    spy_record      rec  = spy_record_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    action_execute_result res = action_execute(&table, 9999u, &rec);
    mu_assert_int_eq(ACTION_EXECUTE_RESULT_VOID, res.type);
    mu_assert_int_eq(0, rec.call_count);
    action_destroy(&table);
}

MU_TEST(test_handle_execute_null_table_is_void) {
    action_execute_result res = action_execute(NULL, 1u, NULL);
    mu_assert_int_eq(ACTION_EXECUTE_RESULT_VOID, res.type);
}

MU_TEST(test_handle_invalidated_by_unrelated_unbind) {
    action_table table;
    action_init(&table);
    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    action_handle duck_before =
         action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'B', "duck", &exec);
    action_unbind_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'A');
    action_handle duck_after = action_resolve_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'B');
    // documented behaviour: unbinding compacts storage, so earlier handles shift
    mu_check(duck_before != duck_after);
    mu_check(duck_after != ACTION_HANDLE_INVALID);
    action_destroy(&table);
}

MU_TEST_SUITE(action_handle_suite) {
    MU_RUN_TEST(test_handle_bind_returns_valid_handle);
    MU_RUN_TEST(test_handle_bind_duplicate_key_returns_invalid);
    MU_RUN_TEST(test_handle_resolve_by_name_and_key_agree);
    MU_RUN_TEST(test_handle_resolve_missing_returns_invalid);
    MU_RUN_TEST(test_handle_execute_passes_self_back);
    MU_RUN_TEST(test_handle_execute_zero_is_void);
    MU_RUN_TEST(test_handle_execute_out_of_range_is_void);
    MU_RUN_TEST(test_handle_execute_null_table_is_void);
    MU_RUN_TEST(test_handle_invalidated_by_unrelated_unbind);
}

MU_TEST_SUITE(action_suite) {
    MU_RUN_TEST(test_init_creates_empty_table);
    MU_RUN_TEST(test_init_null_table_safe);
    MU_RUN_TEST(test_destroy_null_table_safe);

    MU_RUN_TEST(test_bind_adds_action);
    MU_RUN_TEST(test_bind_rejects_duplicate_key);
    MU_RUN_TEST(test_bind_allows_same_key_different_context);
    MU_RUN_TEST(test_bind_rejects_name_too_long);
    MU_RUN_TEST(test_bind_null_table_returns_false);
    MU_RUN_TEST(test_bind_null_executor_returns_false);

    MU_RUN_TEST(test_resolve_by_name_finds_match);
    MU_RUN_TEST(test_resolve_by_name_no_match_returns_null);
    MU_RUN_TEST(test_resolve_by_name_null_table_returns_null);
    MU_RUN_TEST(test_resolve_by_key_finds_match);
    MU_RUN_TEST(test_resolve_by_key_no_match_returns_null);
    MU_RUN_TEST(test_resolve_by_key_null_table_returns_null);

    MU_RUN_TEST(test_rebind_changes_key);
    MU_RUN_TEST(test_rebind_no_match_returns_false);
    MU_RUN_TEST(test_rebind_null_table_returns_false);

    MU_RUN_TEST(test_unbind_by_key_removes_entry);
    MU_RUN_TEST(test_unbind_by_key_no_match_returns_false);
    MU_RUN_TEST(test_unbind_by_key_leaves_others_intact);
    MU_RUN_TEST(test_unbind_by_key_null_table_returns_false);

    MU_RUN_TEST(test_unbind_by_name_removes_entry);
    MU_RUN_TEST(test_unbind_by_name_no_match_returns_false);
    MU_RUN_TEST(test_unbind_by_name_leaves_others_intact);
    MU_RUN_TEST(test_unbind_by_name_null_table_returns_false);

    MU_RUN_TEST(test_unbind_all_removes_all_matching);
    MU_RUN_TEST(test_unbind_all_filters_by_context_and_type);
    MU_RUN_TEST(test_unbind_all_no_match_returns_false);
    MU_RUN_TEST(test_unbind_all_null_table_returns_false);

    MU_RUN_TEST(test_execute_null_action_returns_void_result);
    MU_RUN_TEST(test_execute_calls_executor_with_action_and_userdata);
    MU_RUN_TEST(test_execute_returns_executor_result);
    MU_RUN_TEST(test_execute_by_name_finds_and_calls);
    MU_RUN_TEST(test_execute_by_name_no_match_returns_void_result);
    MU_RUN_TEST(test_execute_by_name_null_table_returns_void_result);

    MU_RUN_TEST(test_integration_multiple_actions_resolve_and_execute_correct_target);
    MU_RUN_TEST(test_integration_full_flow);
}

int main(void) {
    MU_RUN_SUITE(action_suite);
    MU_RUN_SUITE(action_handle_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
