#include "../../thirdparty/minunit.h"

#include <math.h>
#include <stdbool.h>

#define NGINE_UNIT_TEST
#undef ZOD_NGINE_IMPLEMENTATION
#define ZOD_NGINE_IMPLEMENTATION
#include "../index.h"

#define CLOSE(a, b) (fabsf((a) - (b)) < 1e-4f)

MU_TEST(test_clip_quad_fully_inside_is_unchanged) {
    float x0, y0, x1, y1, u0, v0, u1, v1;
    bool  ok = render_text_clip_quad(10.0f, 20.0f, 30.0f, 50.0f, 0.1f, 0.2f, 0.3f, 0.5f,
                                     0.0f, 0.0f, 100.0f, 100.0f, &x0, &y0, &x1, &y1, &u0,
                                     &v0, &u1, &v1);
    mu_check(ok);
    mu_check(CLOSE(x0, 10.0f) && CLOSE(y0, 20.0f) && CLOSE(x1, 30.0f) &&
             CLOSE(y1, 50.0f));
    mu_check(CLOSE(u0, 0.1f) && CLOSE(v0, 0.2f) && CLOSE(u1, 0.3f) && CLOSE(v1, 0.5f));
}

MU_TEST(test_clip_quad_unbounded_rect_never_clips) {
    float x0, y0, x1, y1, u0, v0, u1, v1;
    bool  ok = render_text_clip_quad(10.0f, 20.0f, 30.0f, 50.0f, 0.1f, 0.2f, 0.3f, 0.5f,
                                     -INFINITY, -INFINITY, INFINITY, INFINITY, &x0, &y0,
                                     &x1, &y1, &u0, &v0, &u1, &v1);
    mu_check(ok);
    mu_check(CLOSE(x0, 10.0f) && CLOSE(x1, 30.0f));
}

MU_TEST(test_clip_quad_fully_outside_returns_false) {
    float x0, y0, x1, y1, u0, v0, u1, v1;
    bool  ok = render_text_clip_quad(10.0f, 20.0f, 30.0f, 50.0f, 0.1f, 0.2f, 0.3f, 0.5f,
                                     100.0f, 100.0f, 200.0f, 200.0f, &x0, &y0, &x1, &y1,
                                     &u0, &v0, &u1, &v1);
    mu_check(!ok);
}

MU_TEST(test_clip_quad_touching_edge_returns_false) {
    float x0, y0, x1, y1, u0, v0, u1, v1;
    bool  ok = render_text_clip_quad(10.0f, 20.0f, 30.0f, 50.0f, 0.1f, 0.2f, 0.3f, 0.5f,
                                     30.0f, 0.0f, 100.0f, 100.0f, &x0, &y0, &x1, &y1, &u0,
                                     &v0, &u1, &v1);
    mu_check(!ok);
}

MU_TEST(test_clip_quad_trims_right_edge_and_remaps_u) {
    float x0, y0, x1, y1, u0, v0, u1, v1;
    bool  ok = render_text_clip_quad(10.0f, 20.0f, 30.0f, 50.0f, 0.1f, 0.2f, 0.3f, 0.5f,
                                     0.0f, 0.0f, 20.0f, 100.0f, &x0, &y0, &x1, &y1, &u0,
                                     &v0, &u1, &v1);
    mu_check(ok);
    mu_check(CLOSE(x0, 10.0f) && CLOSE(x1, 20.0f));
    mu_check(CLOSE(u0, 0.1f) && CLOSE(u1, 0.2f));
    mu_check(CLOSE(v0, 0.2f) && CLOSE(v1, 0.5f));  
}

MU_TEST(test_clip_quad_trims_bottom_edge_and_remaps_v) {
    float x0, y0, x1, y1, u0, v0, u1, v1;
    bool  ok = render_text_clip_quad(10.0f, 20.0f, 30.0f, 50.0f, 0.1f, 0.2f, 0.3f, 0.5f,
                                     0.0f, 0.0f, 100.0f, 35.0f, &x0, &y0, &x1, &y1, &u0,
                                     &v0, &u1, &v1);
    mu_check(ok);
    mu_check(CLOSE(y0, 20.0f) && CLOSE(y1, 35.0f));
    mu_check(CLOSE(v0, 0.2f) && CLOSE(v1, 0.35f));
    mu_check(CLOSE(x0, 10.0f) && CLOSE(x1, 30.0f));
}

MU_TEST(test_clip_quad_trims_left_and_top_edges) {
    float x0, y0, x1, y1, u0, v0, u1, v1;
    bool ok = render_text_clip_quad(10.0f, 20.0f, 30.0f, 50.0f, 0.1f, 0.2f, 0.3f, 0.5f,
                                    20.0f, 35.0f, 100.0f, 100.0f, &x0, &y0, &x1, &y1, &u0,
                                    &v0, &u1, &v1);
    mu_check(ok);
    mu_check(CLOSE(x0, 20.0f) && CLOSE(x1, 30.0f));
    mu_check(CLOSE(y0, 35.0f) && CLOSE(y1, 50.0f));
    mu_check(CLOSE(u0, 0.2f) && CLOSE(u1, 0.3f));
    mu_check(CLOSE(v0, 0.35f) && CLOSE(v1, 0.5f));
}

MU_TEST_SUITE(render_text_clip_suite) {
    MU_RUN_TEST(test_clip_quad_fully_inside_is_unchanged);
    MU_RUN_TEST(test_clip_quad_unbounded_rect_never_clips);
    MU_RUN_TEST(test_clip_quad_fully_outside_returns_false);
    MU_RUN_TEST(test_clip_quad_touching_edge_returns_false);
    MU_RUN_TEST(test_clip_quad_trims_right_edge_and_remaps_u);
    MU_RUN_TEST(test_clip_quad_trims_bottom_edge_and_remaps_v);
    MU_RUN_TEST(test_clip_quad_trims_left_and_top_edges);
}

int main(void) {
    MU_RUN_SUITE(render_text_clip_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
