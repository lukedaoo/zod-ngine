#include "../../thirdparty/minunit.h"

#include <stdbool.h>

#define NGINE_UNIT_TEST
#undef ZOD_NGINE_IMPLEMENTATION
#define ZOD_NGINE_IMPLEMENTATION
#include "../index.h"

enum : uint8_t { TEST_CONTEXT_A = 0, TEST_CONTEXT_B = 1 } test_contexts;
enum : uint8_t { TEST_EVENT_CATEGORY = 5 } test_event_category;
enum : uint8_t { TEST_EVENT_ID = 100 } test_event_ids;

static int action_hits_a;
static int action_hits_b;

static action_execute_result action_exec_a(const action_table *table, action_handle self,
                                           void *userdata) {
    (void)table;
    (void)self;
    if (userdata) *(int *)userdata += 1;
    action_hits_a++;
    return (action_execute_result){.type = ACTION_EXECUTE_RESULT_INT, .value.i = 1};
}

static action_execute_result action_exec_b(const action_table *table, action_handle self,
                                           void *userdata) {
    (void)table;
    (void)self;
    (void)userdata;
    action_hits_b++;
    return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
}

static int event_hits_a;
static int event_hits_b;
static int event_last_payload;

static event_callback_result event_cb_a(event_context *ctx, void *userdata) {
    (void)userdata;
    event_hits_a++;
    if (ctx->payload) event_last_payload = *(const int *)ctx->payload;
    return (event_callback_result){.type = EVENT_CALLBACK_RESULT_VOID};
}

static event_callback_result event_cb_b(event_context *ctx, void *userdata) {
    (void)ctx;
    (void)userdata;
    event_hits_b++;
    return (event_callback_result){.type = EVENT_CALLBACK_RESULT_VOID};
}

static bool stub_load_config(const char *path, cvar_table *cvars) {
    (void)path;
    (void)cvars;
    return true;
}

static void press_key(zod_key_t key) {
    g_ctx.input.prev[key] = false;
    g_ctx.input.curr[key] = true;
}

static void hold_key(zod_key_t key) {
    g_ctx.input.prev[key] = true;
    g_ctx.input.curr[key] = true;
}

static void release_key(zod_key_t key) {
    g_ctx.input.prev[key] = true;
    g_ctx.input.curr[key] = false;
}

static void setup(void) {
    zngine_init((zngine_init_params){
         .config_setup = {.load_config_func = stub_load_config, .config_path = "/dev/null"},
    });
    action_hits_a      = 0;
    action_hits_b      = 0;
    event_hits_a       = 0;
    event_hits_b       = 0;
    event_last_payload = 0;
}

static void teardown(void) { zngine_destroy(); }

MU_TEST(test_action_bind_returns_valid_handle) {
    action_executor executor = {.execute = action_exec_a};
    action_handle   handle   = zngine_action_bind(TEST_CONTEXT_A,
                                                  ACTION_TRIGGER_KEY_PRESSED,
                                                  SDL_SCANCODE_A, "jump", &executor);
    mu_check(handle != ACTION_HANDLE_INVALID);
    mu_check(zngine_action_resolve_by_name(TEST_CONTEXT_A, ACTION_TRIGGER_KEY_PRESSED,
                                           "jump") == handle);
    mu_check(zngine_action_resolve_by_key(TEST_CONTEXT_A, ACTION_TRIGGER_KEY_PRESSED,
                                          SDL_SCANCODE_A) == handle);
}

MU_TEST(test_action_bind_duplicate_key_rejected) {
    action_executor executor = {.execute = action_exec_a};
    mu_check(zngine_action_bind(TEST_CONTEXT_A, ACTION_TRIGGER_KEY_PRESSED,
                                SDL_SCANCODE_A, "jump", &executor) !=
             ACTION_HANDLE_INVALID);
    mu_check(zngine_action_bind(TEST_CONTEXT_A, ACTION_TRIGGER_KEY_PRESSED,
                                SDL_SCANCODE_A, "duck", &executor) ==
             ACTION_HANDLE_INVALID);
}

MU_TEST(test_action_same_key_distinct_contexts) {
    action_executor executor_a = {.execute = action_exec_a};
    action_executor executor_b = {.execute = action_exec_b};
    mu_check(zngine_action_bind(TEST_CONTEXT_A, ACTION_TRIGGER_KEY_PRESSED,
                                SDL_SCANCODE_A, "jump", &executor_a) !=
             ACTION_HANDLE_INVALID);
    mu_check(zngine_action_bind(TEST_CONTEXT_B, ACTION_TRIGGER_KEY_PRESSED,
                                SDL_SCANCODE_A, "jump", &executor_b) !=
             ACTION_HANDLE_INVALID);

    press_key(SDL_SCANCODE_A);
    mu_assert_int_eq(1, (int)zngine_action_dispatch(TEST_CONTEXT_A, NULL));
    mu_assert_int_eq(1, action_hits_a);
    mu_assert_int_eq(0, action_hits_b);

    mu_assert_int_eq(1, (int)zngine_action_dispatch(TEST_CONTEXT_B, NULL));
    mu_assert_int_eq(1, action_hits_b);
}

MU_TEST(test_action_execute_passes_userdata) {
    int             counter  = 0;
    action_executor executor = {.execute = action_exec_a};
    action_handle   handle   = zngine_action_bind(TEST_CONTEXT_A,
                                                  ACTION_TRIGGER_KEY_PRESSED,
                                                  SDL_SCANCODE_A, "jump", &executor);

    action_execute_result result = zngine_action_execute(handle, &counter);
    mu_check(result.type == ACTION_EXECUTE_RESULT_INT);
    mu_assert_int_eq(1, result.value.i);
    mu_assert_int_eq(1, counter);
    mu_assert_int_eq(1, action_hits_a);
}

MU_TEST(test_action_dispatch_fires_only_on_press_edge) {
    action_executor executor = {.execute = action_exec_a};
    zngine_action_bind(TEST_CONTEXT_A, ACTION_TRIGGER_KEY_PRESSED, SDL_SCANCODE_A, "jump",
                       &executor);

    press_key(SDL_SCANCODE_A);
    zngine_action_dispatch(TEST_CONTEXT_A, NULL);
    mu_assert_int_eq(1, action_hits_a);

    hold_key(SDL_SCANCODE_A);
    zngine_action_dispatch(TEST_CONTEXT_A, NULL);
    mu_assert_int_eq(1, action_hits_a);

    release_key(SDL_SCANCODE_A);
    zngine_action_dispatch(TEST_CONTEXT_A, NULL);
    mu_assert_int_eq(1, action_hits_a);
}

MU_TEST(test_action_dispatch_key_down_and_released_triggers) {
    action_executor executor_down     = {.execute = action_exec_a};
    action_executor executor_released = {.execute = action_exec_b};
    zngine_action_bind(TEST_CONTEXT_A, ACTION_TRIGGER_KEY_DOWN, SDL_SCANCODE_A, "hold",
                       &executor_down);
    zngine_action_bind(TEST_CONTEXT_A, ACTION_TRIGGER_KEY_RELEASED, SDL_SCANCODE_A,
                       "let_go", &executor_released);

    hold_key(SDL_SCANCODE_A);
    zngine_action_dispatch(TEST_CONTEXT_A, NULL);
    mu_assert_int_eq(1, action_hits_a);
    mu_assert_int_eq(0, action_hits_b);

    release_key(SDL_SCANCODE_A);
    zngine_action_dispatch(TEST_CONTEXT_A, NULL);
    mu_assert_int_eq(1, action_hits_a);
    mu_assert_int_eq(1, action_hits_b);
}

MU_TEST(test_action_dispatch_ignores_unpressed_keys) {
    action_executor executor = {.execute = action_exec_a};
    zngine_action_bind(TEST_CONTEXT_A, ACTION_TRIGGER_KEY_PRESSED, SDL_SCANCODE_A, "jump",
                       &executor);

    press_key(SDL_SCANCODE_B);
    mu_assert_int_eq(0, (int)zngine_action_dispatch(TEST_CONTEXT_A, NULL));
    mu_assert_int_eq(0, action_hits_a);
}

MU_TEST(test_action_unbind_stops_dispatch) {
    action_executor executor = {.execute = action_exec_a};
    zngine_action_bind(TEST_CONTEXT_A, ACTION_TRIGGER_KEY_PRESSED, SDL_SCANCODE_A, "jump",
                       &executor);
    mu_check(zngine_action_unbind_by_name(TEST_CONTEXT_A, ACTION_TRIGGER_KEY_PRESSED,
                                          "jump"));

    press_key(SDL_SCANCODE_A);
    mu_assert_int_eq(0, (int)zngine_action_dispatch(TEST_CONTEXT_A, NULL));
    mu_assert_int_eq(0, action_hits_a);
}

MU_TEST(test_event_emit_reaches_all_subscribers) {
    zngine_event_subscribe(TEST_EVENT_CATEGORY, TEST_EVENT_ID, event_cb_a, NULL, NULL);
    zngine_event_subscribe(TEST_EVENT_CATEGORY, TEST_EVENT_ID, event_cb_b, NULL, NULL);
    zngine_event_subscribe(TEST_EVENT_CATEGORY, TEST_EVENT_ID, event_cb_a, NULL, NULL);

    int payload = 7;
    zngine_event_emit(TEST_EVENT_CATEGORY, TEST_EVENT_ID, &payload, sizeof(payload));

    mu_assert_int_eq(2, event_hits_a);
    mu_assert_int_eq(1, event_hits_b);
    mu_assert_int_eq(7, event_last_payload);
}

MU_TEST(test_event_emit_skips_other_identifiers) {
    zngine_event_subscribe(TEST_EVENT_CATEGORY, TEST_EVENT_ID, event_cb_a, NULL, NULL);

    zngine_event_emit(TEST_EVENT_CATEGORY, TEST_EVENT_ID + 1, NULL, 0);
    zngine_event_emit(TEST_EVENT_CATEGORY + 1, TEST_EVENT_ID, NULL, 0);

    mu_assert_int_eq(0, event_hits_a);
}

MU_TEST(test_event_unsubscribe_leaves_other_subscribers) {
    const event_handle handle_a =
         zngine_event_subscribe(TEST_EVENT_CATEGORY, TEST_EVENT_ID, event_cb_a, NULL, NULL);
    zngine_event_subscribe(TEST_EVENT_CATEGORY, TEST_EVENT_ID, event_cb_b, NULL, NULL);

    mu_check(zngine_event_unsubscribe(handle_a));
    zngine_event_emit(TEST_EVENT_CATEGORY, TEST_EVENT_ID, NULL, 0);

    mu_assert_int_eq(0, event_hits_a);
    mu_assert_int_eq(1, event_hits_b);
}

MU_TEST(test_event_publish_and_emit_are_equivalent) {
    zngine_event_subscribe(TEST_EVENT_CATEGORY, TEST_EVENT_ID, event_cb_a, NULL, NULL);
    zngine_event_subscribe(TEST_EVENT_CATEGORY, TEST_EVENT_ID, event_cb_b, NULL, NULL);

    int           payload = 7;
    event_context ctx     = {.identifier   = {.category = TEST_EVENT_CATEGORY,
                                              .event_id = TEST_EVENT_ID},
                             .payload      = &payload,
                             .payload_size = sizeof(payload)};
    zngine_event_publish(&ctx);
    zngine_event_emit(TEST_EVENT_CATEGORY, TEST_EVENT_ID, &payload, sizeof(payload));

    mu_assert_int_eq(2, event_hits_a);
    mu_assert_int_eq(2, event_hits_b);
    mu_assert_int_eq(7, event_last_payload);
}

MU_TEST_SUITE(action_suite) {
    MU_SUITE_CONFIGURE(&setup, &teardown);
    MU_RUN_TEST(test_action_bind_returns_valid_handle);
    MU_RUN_TEST(test_action_bind_duplicate_key_rejected);
    MU_RUN_TEST(test_action_same_key_distinct_contexts);
    MU_RUN_TEST(test_action_execute_passes_userdata);
    MU_RUN_TEST(test_action_dispatch_fires_only_on_press_edge);
    MU_RUN_TEST(test_action_dispatch_key_down_and_released_triggers);
    MU_RUN_TEST(test_action_dispatch_ignores_unpressed_keys);
    MU_RUN_TEST(test_action_unbind_stops_dispatch);
}

MU_TEST_SUITE(event_suite) {
    MU_SUITE_CONFIGURE(&setup, &teardown);
    MU_RUN_TEST(test_event_emit_reaches_all_subscribers);
    MU_RUN_TEST(test_event_emit_skips_other_identifiers);
    MU_RUN_TEST(test_event_unsubscribe_leaves_other_subscribers);
    MU_RUN_TEST(test_event_publish_and_emit_are_equivalent);
}

int main(void) {
    MU_RUN_SUITE(action_suite);
    MU_RUN_SUITE(event_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
