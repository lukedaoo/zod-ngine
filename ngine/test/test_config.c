#include "../../lib/minunit.h"

#include <stdbool.h>

#define ZOD_NGINE_IMPLEMENTATION
#include "../index.h"

static void reset(void) {
    cvar_destroy(&g_config_storage.cvars);
    g_config_storage = (g_config){0};
    g_config_seed_preset(&g_config_storage);
}

MU_TEST(test_preset_int_defaults) {
    reset();
    mu_assert_int_eq(800, config_get_int("window.width", 0));
    mu_assert_int_eq(600, config_get_int("window.height", 0));
    mu_assert_int_eq(0, config_get_int("log.level", -1));
}

MU_TEST(test_preset_string_default) {
    reset();
    mu_assert_string_eq("zod-ngine", config_get_string("window.title", ""));
}

MU_TEST(test_preset_bool_default) {
    reset();
    mu_check(config_get_bool("window.vsync", false) == true);
}

MU_TEST(test_fallback_on_missing) {
    reset();
    mu_assert_int_eq(42, config_get_int("no.such.key", 42));
    mu_check(config_get_float("no.such.key", 1.5f) == 1.5f);
    mu_check(config_get_bool("no.such.key", true) == true);
    mu_assert_string_eq("fb", config_get_string("no.such.key", "fb"));
}

MU_TEST(test_set_get_roundtrip) {
    reset();
    mu_check(config_set_int("window.width", 1920));
    mu_assert_int_eq(1920, config_get_int("window.width", 0));

    mu_check(config_set_bool("window.vsync", false));
    mu_check(config_get_bool("window.vsync", true) == false);

    mu_check(config_set_string("window.title", "custom"));
    mu_assert_string_eq("custom", config_get_string("window.title", ""));
}

MU_TEST(test_engine_config_valid_without_cvars) {
    cvar_destroy(&g_config_storage.cvars);
    g_config_storage = (g_config){0};
    g_ctx.config     = &g_config_storage;

    mu_check(g_ctx.config != NULL);
    mu_check(g_ctx.config->cvars.data == NULL);

    mu_assert_int_eq(99, config_get_int("window.width", 99));
    mu_check(config_get_float("window.height", 1.0f) == 1.0f);
    mu_check(config_get_bool("window.vsync", false) == false);
    mu_assert_string_eq("fb", config_get_string("window.title", "fb"));
}

MU_TEST_SUITE(config_suite) {
    MU_RUN_TEST(test_preset_int_defaults);
    MU_RUN_TEST(test_preset_string_default);
    MU_RUN_TEST(test_preset_bool_default);
    MU_RUN_TEST(test_fallback_on_missing);
    MU_RUN_TEST(test_set_get_roundtrip);
    MU_RUN_TEST(test_engine_config_valid_without_cvars);
}

int main(void) {
    MU_RUN_SUITE(config_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
