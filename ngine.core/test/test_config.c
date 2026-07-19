#include "../../thirdparty/minunit.h"

#include <stdbool.h>

#define NGINE_UNIT_TEST
#undef ZOD_NGINE_IMPLEMENTATION
#define ZOD_NGINE_IMPLEMENTATION
#include "../index.h"

static void reset(void) {
    cvar_destroy(&g_ctx.config.cvars);
    g_ctx = (engine_context){0};
    config_seed_preset(&g_ctx.config);
}

MU_TEST(test_preset_int_defaults) {
    reset();
    mu_assert_int_eq(800, zod_config_get_int("window.width", 0));
    mu_assert_int_eq(600, zod_config_get_int("window.height", 0));
    mu_assert_int_eq(0, zod_config_get_int("log.level", -1));
}

MU_TEST(test_preset_string_default) {
    reset();
    mu_assert_string_eq("zod-ngine", zod_config_get_string("window.title", ""));
}

MU_TEST(test_preset_bool_default) {
    reset();
    mu_check(zod_config_get_bool("window.vsync", false) == true);
}

MU_TEST(test_fallback_on_missing) {
    reset();
    mu_assert_int_eq(42, zod_config_get_int("no.such.key", 42));
    mu_check(zod_config_get_float("no.such.key", 1.5f) == 1.5f);
    mu_check(zod_config_get_bool("no.such.key", true) == true);
    mu_assert_string_eq("fb", zod_config_get_string("no.such.key", "fb"));
}

MU_TEST(test_set_get_roundtrip) {
    reset();
    mu_check(zod_config_set_int("window.width", 1920));
    mu_assert_int_eq(1920, zod_config_get_int("window.width", 0));

    mu_check(zod_config_set_bool("window.vsync", false));
    mu_check(zod_config_get_bool("window.vsync", true) == false);

    mu_check(zod_config_set_string("window.title", "custom"));
    mu_assert_string_eq("custom", zod_config_get_string("window.title", ""));
}

MU_TEST(test_engine_config_valid_without_cvars) {
    cvar_destroy(&g_ctx.config.cvars);
    g_ctx = (engine_context){0};

    mu_check(g_ctx.config.cvars.data == NULL);

    mu_assert_int_eq(99, zod_config_get_int("window.width", 99));
    mu_check(zod_config_get_float("window.height", 1.0f) == 1.0f);
    mu_check(zod_config_get_bool("window.vsync", false) == false);
    mu_assert_string_eq("fb", zod_config_get_string("window.title", "fb"));
}

MU_TEST(test_adjust_config_converts_string_debug_to_int) {
    reset();
    cvar_set_string(&g_ctx.config.cvars, "log.level", "debug");
    mu_assert_int_eq(-1, zod_config_get_int("log.level", -1));
    mu_check(config_adjust(&g_ctx.config));
    mu_assert_int_eq(LOG_DEBUG, zod_config_get_int("log.level", -1));
}

MU_TEST(test_adjust_config_leaves_int_unchanged) {
    reset();
    mu_check(config_adjust(&g_ctx.config));
    mu_assert_int_eq(LOG_TRACE, zod_config_get_int("log.level", -1));
}

MU_TEST(test_adjust_config_all_string_levels) {
    static const struct {
        const char *s;
        int         v;
    } cases[] = {
         {"trace", LOG_TRACE}, {"debug", LOG_DEBUG}, {"info", LOG_INFO},
         {"warn", LOG_WARN},   {"error", LOG_ERROR}, {"fatal", LOG_FATAL},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        reset();
        cvar_set_string(&g_ctx.config.cvars, "log.level", cases[i].s);
        mu_check(config_adjust(&g_ctx.config));
        mu_assert_int_eq(cases[i].v, zod_config_get_int("log.level", -1));
    }
}

MU_TEST(test_adjust_config_null_returns_false) { mu_check(!config_adjust(NULL)); }

static bool reload_succeeds(const char *path, cvar_table *cvars) {
    (void)path;
    cvar_set_int(cvars, "window.width", 1280);
    cvar_set_int(cvars, "window.height", 720);
    return true;
}

static bool reload_fails(const char *path, cvar_table *cvars) {
    (void)path;
    (void)cvars;
    return false;
}

static bool reload_partial_then_fail(const char *path, cvar_table *cvars) {
    (void)path;
    cvar_set_int(cvars, "window.width", 9999);
    return false;
}

MU_TEST(test_reload_no_watcher_returns_false) {
    reset();
    g_ctx.config.config_file_watcher = NULL;
    mu_check(!config_reload_from_file(&g_ctx.config));
}

MU_TEST(test_reload_success_updates_config) {
    reset();
    file_watcher fw                  = {.path = "fake.scf"};
    g_ctx.config.config_file_watcher = &fw;
    g_ctx.config.reload_config_func  = reload_succeeds;

    mu_check(config_reload_from_file(&g_ctx.config));
    mu_assert_int_eq(1280, zod_config_get_int("window.width", 0));
    mu_assert_int_eq(720, zod_config_get_int("window.height", 0));
    mu_assert_int_eq(60, zod_config_get_int("engine.target_fps", 0));

    g_ctx.config.config_file_watcher = NULL;
}

MU_TEST(test_reload_fail_preserves_config) {
    reset();
    zod_config_set_int("window.width", 1920);

    file_watcher fw                  = {.path = "fake.scf"};
    g_ctx.config.config_file_watcher = &fw;
    g_ctx.config.reload_config_func  = reload_fails;

    mu_check(!config_reload_from_file(&g_ctx.config));
    mu_assert_int_eq(1920, zod_config_get_int("window.width", 0));

    g_ctx.config.config_file_watcher = NULL;
}

MU_TEST(test_reload_partial_fail_preserves_config) {
    reset();
    zod_config_set_int("window.width", 1920);

    file_watcher fw                  = {.path = "fake.scf"};
    g_ctx.config.config_file_watcher = &fw;
    g_ctx.config.reload_config_func  = reload_partial_then_fail;

    mu_check(!config_reload_from_file(&g_ctx.config));
    mu_assert_int_eq(1920, zod_config_get_int("window.width", 0));

    g_ctx.config.config_file_watcher = NULL;
}

MU_TEST_SUITE(config_suite) {
    MU_RUN_TEST(test_preset_int_defaults);
    MU_RUN_TEST(test_preset_string_default);
    MU_RUN_TEST(test_preset_bool_default);
    MU_RUN_TEST(test_fallback_on_missing);
    MU_RUN_TEST(test_set_get_roundtrip);
    MU_RUN_TEST(test_engine_config_valid_without_cvars);
    MU_RUN_TEST(test_adjust_config_converts_string_debug_to_int);
    MU_RUN_TEST(test_adjust_config_leaves_int_unchanged);
    MU_RUN_TEST(test_adjust_config_all_string_levels);
    MU_RUN_TEST(test_adjust_config_null_returns_false);
    MU_RUN_TEST(test_reload_no_watcher_returns_false);
    MU_RUN_TEST(test_reload_success_updates_config);
    MU_RUN_TEST(test_reload_fail_preserves_config);
    MU_RUN_TEST(test_reload_partial_fail_preserves_config);
}

int main(void) {
    MU_RUN_SUITE(config_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
