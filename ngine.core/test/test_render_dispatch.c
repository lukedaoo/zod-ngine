#include "../../thirdparty/minunit.h"

#include <stdbool.h>

#define NGINE_UNIT_TEST
#undef ZOD_NGINE_IMPLEMENTATION
#define ZOD_NGINE_IMPLEMENTATION
#include "../index.h"

MU_TEST(test_context_create_returns_non_null) {
    void *render_context = render_backend_context_create(NULL);
    mu_check(render_context != NULL);
}

MU_TEST(test_backend_type_matches_compiled_backend) {
    mu_assert_int_eq(RENDER_BACKEND, (int)BACKEND_TYPE_OPENGL);
}

MU_TEST_SUITE(render_dispatch_suite) {
    MU_RUN_TEST(test_context_create_returns_non_null);
    MU_RUN_TEST(test_backend_type_matches_compiled_backend);
}

int main(void) {
    MU_RUN_SUITE(render_dispatch_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
