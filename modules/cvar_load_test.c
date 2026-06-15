#include "../lib/minunit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CVAR_IMPLEMENTATION
#include "cvar.h"

#define INI_IMPLEMENTATION
#include "ini.h"

#define CVAR_LOAD_IMPLEMENTATION
#include "cvar_load.h"

#ifdef _WIN32
#define TEST_INI "tmp/cvar_load_test.ini"
#else
#define TEST_INI "/tmp/cvar_load_test.ini"
#endif

static void write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    fputs(content, f);
    fclose(f);
}

static bool test_ini_handler(const char *section,
                             const char *key,
                             const char *value,
                             void       *user) {
    cvar_table *cvars = user;

#define MATCH(s, k) (strcmp(section, s) == 0 && strcmp(key, k) == 0)
    if (MATCH("test", "value")) {
        int v = atoi(value);
        cvar_set(cvars, "test.value", CVAR_INT, &v);
    } else if (MATCH("test", "name")) {
        cvar_set(cvars, "test.name", CVAR_STRING, (void *)value);
    } else {
        return false;
    }
    return true;
#undef MATCH
}

MU_TEST(test_cvar_reload_ini_success) {
    cvar_table table = {0};

    write_file(TEST_INI, "[test]\nvalue=1\nname=alice\n");
    mu_check(ini_parse(TEST_INI, test_ini_handler, &table));

    write_file(TEST_INI, "[test]\nvalue=2\nname=bob\n");
    mu_check(cvar_load_ini(&table, TEST_INI, test_ini_handler));

    cvar_t *v = cvar_get(&table, "test.value");
    mu_check(v->value.i == 2);

    cvar_t *n = cvar_get(&table, "test.name");
    mu_assert_string_eq("bob", n->value.s);

    cvar_destroy(&table);
    remove(TEST_INI);
}

MU_TEST(test_cvar_reload_ini_failure_keeps_old) {
    cvar_table table = {0};

    write_file(TEST_INI, "[test]\nvalue=1\nname=alice\n");
    mu_check(ini_parse(TEST_INI, test_ini_handler, &table));

    write_file(TEST_INI, "[test]\nvalue=99\nunknown_key=oops\n");
    mu_check(!cvar_load_ini(&table, TEST_INI, test_ini_handler));

    cvar_t *v = cvar_get(&table, "test.value");
    mu_check(v->value.i == 1);

    cvar_t *n = cvar_get(&table, "test.name");
    mu_assert_string_eq("alice", n->value.s);

    cvar_destroy(&table);
    remove(TEST_INI);
}

MU_TEST_SUITE(cvar_reload_suite) {
    MU_RUN_TEST(test_cvar_reload_ini_success);
    MU_RUN_TEST(test_cvar_reload_ini_failure_keeps_old);
}

int main(void) {
    MU_RUN_SUITE(cvar_reload_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
