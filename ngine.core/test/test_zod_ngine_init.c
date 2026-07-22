#include "../../thirdparty/minunit.h"

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#define NGINE_UNIT_TEST
#undef ZOD_NGINE_IMPLEMENTATION
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

static void stub_before_init(void *user_data) {
    (void)user_data;
    before_init_called = true;
}
static void stub_after_init(void *user_data) {
    (void)user_data;
    after_init_called = true;
}

static bool stub_load_config(const char *path, cvar_table *cvars) {
    load_config_called         = true;
    load_config_received_path  = path;
    load_config_received_cvars = cvars;
    return load_config_return_val;
}

static bool stub_load_config_negative_width(const char *path, cvar_table *cvars) {
    (void)path;
    cvar_set_int(cvars, "window.width", -1);
    return true;
}

static bool stub_load_config_negative_height(const char *path, cvar_table *cvars) {
    (void)path;
    cvar_set_int(cvars, "window.height", -1);
    return true;
}

static bool stub_load_config_negative_fps(const char *path, cvar_table *cvars) {
    (void)path;
    cvar_set_int(cvars, "engine.target_fps", -22);
    return true;
}

static bool stub_load_args(int argc, const char **argv, cvar_table *cvars) {
    (void)argc;
    (void)argv;
    (void)cvars;
    load_args_called = true;
    return load_args_return_val;
}

static void order_before_init(void *user_data) {
    (void)user_data;
    call_order[call_count++] = 0;
}
static void order_after_init(void *user_data) {
    (void)user_data;
    call_order[call_count++] = 1;
}

static void reset(void) {
    cvar_destroy(&g_ctx.config.cvars);
    if (g_ctx.config.config_file_watcher)
        file_watcher_close(g_ctx.config.config_file_watcher);
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
    mu_check(zngine_init((zngine_init_params){0}));
}

MU_TEST(test_init_seeds_preset_config) {
    reset();
    zngine_init((zngine_init_params){0});
    mu_assert_int_eq(800, zngine_config_get_int("window.width", 0));
    mu_assert_int_eq(600, zngine_config_get_int("window.height", 0));
    mu_assert_string_eq("zod-ngine", zngine_config_get_string("window.title", ""));
}

MU_TEST(test_init_before_init_hook_fires) {
    reset();
    zngine_init((zngine_init_params){
         .dispatch = {.before_init = stub_before_init},
    });
    mu_check(before_init_called);
}

MU_TEST(test_init_after_init_hook_fires) {
    reset();
    zngine_init((zngine_init_params){
         .dispatch = {.after_init = stub_after_init},
    });
    mu_check(after_init_called);
}

MU_TEST(test_init_hook_order) {
    reset();
    zngine_init((zngine_init_params){
         .dispatch =
              {
                   .before_init = order_before_init,
                   .after_init  = order_after_init,
              },
    });
    mu_assert_int_eq(0, call_order[0]);
    mu_assert_int_eq(1, call_order[1]);
}

MU_TEST(test_init_missing_config_path_returns_true_no_load_config_from_file) {
    reset();
    bool ok = zngine_init((zngine_init_params){
         .config_setup = {.load_config_func = stub_load_config, .config_path = NULL},
    });
    mu_check(ok);
    mu_check(!load_config_called);
}

MU_TEST(test_init_config_load_failure_returns_false) {
    reset();
    load_config_return_val = false;
    bool ok                = zngine_init((zngine_init_params){
         .config_setup = {.load_config_func = stub_load_config,
                          .config_path      = "/dev/null"},
    });
    mu_check(!ok);
    mu_check(load_config_called);
}

MU_TEST(test_init_config_load_receives_correct_args) {
    reset();
    load_config_return_val = true;
    zngine_init((zngine_init_params){
         .config_setup = {.load_config_func = stub_load_config,
                          .config_path      = "/dev/null"},
    });
    mu_check(load_config_called);
    mu_assert_string_eq("/dev/null", load_config_received_path);
    mu_check(load_config_received_cvars == &g_ctx.config.cvars);
}

MU_TEST(test_init_load_args_failure_continues) {
    reset();
    load_args_return_val = false;
    bool ok              = zngine_init((zngine_init_params){
         .dispatch = {.load_args = stub_load_args},
    });
    mu_check(ok);
    mu_check(load_args_called);
}

MU_TEST(test_init_stores_config_in_ctx) {
    reset();
    zngine_init((zngine_init_params){0});
    mu_assert_int_eq(800, zngine_config_get_int("window.width", 0));
}

MU_TEST(test_init_config_load_failure_stops_before_load_args) {
    reset();
    load_config_return_val = false;
    load_args_return_val   = false;
    bool ok                = zngine_init((zngine_init_params){
         .config_setup = {.load_config_func = stub_load_config,
                          .config_path      = "/dev/null"},
         .dispatch     = {.load_args = stub_load_args},
    });
    mu_check(!ok);
    mu_check(load_config_called);
    mu_check(!load_args_called);
}

MU_TEST(test_init_hot_reload_on_failure_no_watcher_attached) {
    reset();
    load_config_return_val = false;
    bool ok                = zngine_init((zngine_init_params){
         .config_setup =
              {
                   .load_config_func = stub_load_config,
                   .config_path      = "/dev/null",
                   .hot_reload       = true,
              },
    });
    mu_check(!ok);
    mu_check(g_ctx.config.reload_config_func == NULL);
    mu_check(g_ctx.config.config_file_watcher == NULL);
}

MU_TEST(test_init_hot_reload_on_success_attaches_watcher) {
    reset();
    load_config_return_val = true;
    zngine_init((zngine_init_params){
         .config_setup =
              {
                   .load_config_func = stub_load_config,
                   .config_path      = "/dev/null",
                   .hot_reload       = true,
              },
    });
    mu_check(g_ctx.config.reload_config_func == stub_load_config);
    mu_check(g_ctx.config.config_file_watcher != NULL);
}

#ifdef _WIN32
#define HOT_RELOAD_TEST_FILE "tmp/zod_hot_reload_test.scf"
#else
#define HOT_RELOAD_TEST_FILE "/tmp/zod_hot_reload_test.scf"
#endif

static void touch(const char *path) {
    FILE *f = fopen(path, "w");
    fclose(f);
}

MU_TEST(test_hot_reload_invalid_config_exits) {
    reset();
    remove(HOT_RELOAD_TEST_FILE);
    touch(HOT_RELOAD_TEST_FILE);

    load_config_return_val = true;
    bool ok                = zngine_init((zngine_init_params){
         .config_setup =
              {
                   .load_config_func = stub_load_config,
                   .config_path      = HOT_RELOAD_TEST_FILE,
                   .hot_reload       = true,
              },
    });
    mu_check(ok);
    mu_check(!zngine_should_exit());

    g_ctx.config.reload_config_func = stub_load_config_negative_width;

    sleep(1);
    touch(HOT_RELOAD_TEST_FILE);

    mu_check(zngine_tick_hot_reload());
    mu_check(!zngine_should_exit());
    mu_assert_int_eq(800, zngine_config_get_int("window.width", -1));

    remove(HOT_RELOAD_TEST_FILE);
}

MU_TEST(test_stage2_load_success_returns_true) {
    reset();
    load_config_return_val = true;
    bool ok                = zngine_init((zngine_init_params){
         .config_setup = {.load_config_func = stub_load_config,
                          .config_path      = "/dev/null"},
    });
    mu_check(ok);
    mu_check(load_config_called);
}

MU_TEST(test_stage2_load_success_hot_reload_attaches_watcher) {
    reset();
    load_config_return_val = true;
    zngine_init((zngine_init_params){
         .config_setup =
              {
                   .load_config_func = stub_load_config,
                   .config_path      = "/dev/null",
                   .hot_reload       = true,
              },
    });
    mu_check(g_ctx.config.config_file_watcher != NULL);
    mu_check(g_ctx.config.reload_config_func == stub_load_config);
}

MU_TEST(test_stage2_load_success_no_hot_reload_no_watcher) {
    reset();
    load_config_return_val = true;
    zngine_init((zngine_init_params){
         .config_setup =
              {
                   .load_config_func = stub_load_config,
                   .config_path      = "/dev/null",
                   .hot_reload       = false,
              },
    });
    mu_check(g_ctx.config.config_file_watcher == NULL);
    mu_check(g_ctx.config.reload_config_func == NULL);
}

MU_TEST(test_stage2_load_fail_no_watcher_attached) {
    reset();
    load_config_return_val = false;
    bool ok                = zngine_init((zngine_init_params){
         .config_setup =
              {
                   .load_config_func = stub_load_config,
                   .config_path      = "/dev/null",
                   .hot_reload       = true,
              },
    });
    mu_check(!ok);
    mu_check(g_ctx.config.config_file_watcher == NULL);
    mu_check(g_ctx.config.reload_config_func == NULL);
}

MU_TEST(test_stage2_load_fail_presets_survive) {
    reset();
    load_config_return_val = false;
    zngine_init((zngine_init_params){
         .config_setup = {.load_config_func = stub_load_config,
                          .config_path      = "/dev/null"},
    });
    mu_assert_int_eq(800, zngine_config_get_int("window.width", 0));
    mu_assert_int_eq(600, zngine_config_get_int("window.height", 0));
    mu_assert_string_eq("zod-ngine", zngine_config_get_string("window.title", ""));
}

MU_TEST(test_init_setter_rejects_negative_width) {
    reset();
    bool ok = zngine_init((zngine_init_params){
         .config_setup = {.load_config_func = stub_load_config_negative_width,
                          .config_path      = "/dev/null"},
    });
    mu_check(ok);
    mu_assert_int_eq(800, zngine_config_get_int("window.width", -1));
}

MU_TEST(test_init_setter_rejects_negative_height) {
    reset();
    bool ok = zngine_init((zngine_init_params){
         .config_setup = {.load_config_func = stub_load_config_negative_height,
                          .config_path      = "/dev/null"},
    });
    mu_check(ok);
    mu_assert_int_eq(600, zngine_config_get_int("window.height", -1));
}

MU_TEST(test_init_setter_rejects_negative_target_fps) {
    reset();
    bool ok = zngine_init((zngine_init_params){
         .config_setup = {.load_config_func = stub_load_config_negative_fps,
                          .config_path      = "/dev/null"},
    });
    mu_check(ok);
    mu_assert_int_eq(60, zngine_config_get_int("engine.target_fps", -1));
}

MU_TEST_SUITE(zod_ngine_init_validation_suite) {
    MU_RUN_TEST(test_init_setter_rejects_negative_width);
    MU_RUN_TEST(test_init_setter_rejects_negative_height);
    MU_RUN_TEST(test_init_setter_rejects_negative_target_fps);
    MU_RUN_TEST(test_hot_reload_invalid_config_exits);
}

MU_TEST_SUITE(zod_ngine_stage2_suite) {
    MU_RUN_TEST(test_stage2_load_success_returns_true);
    MU_RUN_TEST(test_stage2_load_success_hot_reload_attaches_watcher);
    MU_RUN_TEST(test_stage2_load_success_no_hot_reload_no_watcher);
    MU_RUN_TEST(test_stage2_load_fail_no_watcher_attached);
    MU_RUN_TEST(test_stage2_load_fail_presets_survive);
}

MU_TEST_SUITE(zod_ngine_init_suite) {
    MU_RUN_TEST(test_init_returns_true_no_dispatch);
    MU_RUN_TEST(test_init_seeds_preset_config);
    MU_RUN_TEST(test_init_before_init_hook_fires);
    MU_RUN_TEST(test_init_after_init_hook_fires);
    MU_RUN_TEST(test_init_hook_order);
    MU_RUN_TEST(test_init_missing_config_path_returns_true_no_load_config_from_file);
    MU_RUN_TEST(test_init_config_load_failure_returns_false);
    MU_RUN_TEST(test_init_config_load_receives_correct_args);
    MU_RUN_TEST(test_init_load_args_failure_continues);
    MU_RUN_TEST(test_init_stores_config_in_ctx);
    MU_RUN_TEST(test_init_config_load_failure_stops_before_load_args);
    MU_RUN_TEST(test_init_hot_reload_on_failure_no_watcher_attached);
    MU_RUN_TEST(test_init_hot_reload_on_success_attaches_watcher);
}

int main(void) {
    MU_RUN_SUITE(zod_ngine_init_suite);
    MU_RUN_SUITE(zod_ngine_init_validation_suite);
    MU_RUN_SUITE(zod_ngine_stage2_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
