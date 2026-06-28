#include "../../lib/minunit.h"

#include <stdbool.h>

#define ZOD_NGINE_IMPLEMENTATION
#include "../index.h"

static bool        before_init_called;
static bool        after_init_called;
static bool        load_config_called;
static bool        load_args_called;
static const char *load_config_received_path;
static cvar_table *load_config_received_cvars;
static bool        load_config_return_val;
static bool        load_args_return_val;
static int         call_order[2];
static int         call_count;

static void stub_before_init(void) { before_init_called = true; }
static void stub_after_init(void) { after_init_called = true; }

static bool stub_load_config(const char *path, cvar_table *cvars) {
    load_config_called         = true;
    load_config_received_path  = path;
    load_config_received_cvars = cvars;
    return load_config_return_val;
}

static bool stub_load_args(int argc, const char **argv, cvar_table *cvars) {
    (void)argc;
    (void)argv;
    (void)cvars;
    load_args_called = true;
    return load_args_return_val;
}

static void order_before_init(void) { call_order[call_count++] = 0; }
static void order_after_init(void) { call_order[call_count++] = 1; }

static void reset(void) {
    cvar_destroy(&g_config_storage.cvars);
    g_config_storage           = (g_config){0};
    g_ctx                      = (engine_context){0};
    before_init_called         = false;
    after_init_called          = false;
    load_config_called         = false;
    load_args_called           = false;
    load_config_received_path  = NULL;
    load_config_received_cvars = NULL;
    load_config_return_val     = false;
    load_args_return_val       = false;
    call_count                 = 0;
}

MU_TEST(test_init_returns_true_no_dispatch) {
    reset();
    mu_check(zod_ngine_init((zod_engine_init_params){0}));
}

MU_TEST(test_init_seeds_preset_config) {
    reset();
    zod_ngine_init((zod_engine_init_params){0});
    mu_assert_int_eq(800, config_get_int("window.width", 0));
    mu_assert_int_eq(600, config_get_int("window.height", 0));
    mu_assert_string_eq("zod-ngine", config_get_string("window.title", ""));
}

MU_TEST(test_init_before_init_hook_fires) {
    reset();
    zod_ngine_init((zod_engine_init_params){
         .dispatch = {.before_init = stub_before_init},
    });
    mu_check(before_init_called);
}

MU_TEST(test_init_after_init_hook_fires) {
    reset();
    zod_ngine_init((zod_engine_init_params){
         .dispatch = {.after_init = stub_after_init},
    });
    mu_check(after_init_called);
}

MU_TEST(test_init_hook_order) {
    reset();
    zod_ngine_init((zod_engine_init_params){
         .dispatch =
              {
                   .before_init = order_before_init,
                   .after_init  = order_after_init,
              },
    });
    mu_assert_int_eq(0, call_order[0]);
    mu_assert_int_eq(1, call_order[1]);
}

MU_TEST(test_init_missing_config_path_returns_false) {
    reset();
    bool ok = zod_ngine_init((zod_engine_init_params){
         .config_file_setup = {.config_path = NULL},
         .dispatch          = {.load_config_from_file = stub_load_config},
    });
    mu_check(!ok);
    mu_check(!load_config_called);
}

MU_TEST(test_init_config_load_failure_keeps_presets) {
    reset();
    load_config_return_val = false;
    bool ok                = zod_ngine_init((zod_engine_init_params){
         .config_file_setup = {.config_path = "/dev/null"},
         .dispatch          = {.load_config_from_file = stub_load_config},
    });
    mu_check(ok);
    mu_check(load_config_called);
    mu_assert_int_eq(800, config_get_int("window.width", 0));
}

MU_TEST(test_init_config_load_receives_correct_args) {
    reset();
    load_config_return_val = true;
    zod_ngine_init((zod_engine_init_params){
         .config_file_setup = {.config_path = "/dev/null"},
         .dispatch          = {.load_config_from_file = stub_load_config},
    });
    mu_check(load_config_called);
    mu_assert_string_eq("/dev/null", load_config_received_path);
    mu_check(load_config_received_cvars == &g_config_storage.cvars);
}

MU_TEST(test_init_load_args_failure_continues) {
    reset();
    load_args_return_val = false;
    bool ok              = zod_ngine_init((zod_engine_init_params){
         .dispatch = {.load_args = stub_load_args},
    });
    mu_check(ok);
    mu_check(load_args_called);
}

MU_TEST(test_init_stores_config_in_ctx) {
    reset();
    zod_ngine_init((zod_engine_init_params){0});
    mu_check(g_ctx.config == &g_config_storage);
}

MU_TEST(test_init_both_stages_fail_preset_survives) {
    reset();
    load_config_return_val = false;
    load_args_return_val   = false;
    bool ok                = zod_ngine_init((zod_engine_init_params){
         .config_file_setup = {.config_path = "/dev/null"},
         .dispatch =
              {
                   .load_config_from_file = stub_load_config,
                   .load_args             = stub_load_args,
              },
    });
    mu_check(ok);
    mu_check(load_config_called);
    mu_check(load_args_called);
    mu_assert_int_eq(800, config_get_int("window.width", 0));
    mu_assert_int_eq(600, config_get_int("window.height", 0));
    mu_assert_string_eq("zod-ngine", config_get_string("window.title", ""));
    mu_check(g_ctx.config == &g_config_storage);
}

MU_TEST_SUITE(zod_ngine_init_suite) {
    MU_RUN_TEST(test_init_returns_true_no_dispatch);
    MU_RUN_TEST(test_init_seeds_preset_config);
    MU_RUN_TEST(test_init_before_init_hook_fires);
    MU_RUN_TEST(test_init_after_init_hook_fires);
    MU_RUN_TEST(test_init_hook_order);
    MU_RUN_TEST(test_init_missing_config_path_returns_false);
    MU_RUN_TEST(test_init_config_load_failure_keeps_presets);
    MU_RUN_TEST(test_init_config_load_receives_correct_args);
    MU_RUN_TEST(test_init_load_args_failure_continues);
    MU_RUN_TEST(test_init_stores_config_in_ctx);
    MU_RUN_TEST(test_init_both_stages_fail_preset_survives);
}

int main(void) {
    MU_RUN_SUITE(zod_ngine_init_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
