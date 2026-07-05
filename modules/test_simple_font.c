#include "../lib/minunit.h"

#define SIMPLE_FONT_IMPLEMENTATION
#include "simple_font.h"

#define TEST_TTF_PATH "modules/testdata/roboto_slab.ttf"

static uint8_t atlas_pixel(const simple_font_atlas *atlas, const simple_font_glyph *g,
                           int px, int py) {
    return atlas->pixels[(g->y + py) * atlas->width + (g->x + px)];
}

MU_TEST(test_load_null_path_fills_ascii_backend) {
    simple_font font = simple_font_load(NULL);

    mu_check(font.backend == SIMPLE_FONT_BACKEND_ASCII);
    simple_font_atlas atlas = simple_font_get_atlas(&font);
    mu_assert_int_eq(SIMPLE_FONT_ATLAS_WIDTH, atlas.width);
    mu_assert_int_eq(SIMPLE_FONT_ATLAS_HEIGHT, atlas.height);
}

MU_TEST(test_load_nonexistent_path_falls_back_to_ascii) {
    simple_font font = simple_font_load("does/not/exist.ttf");

    mu_check(font.backend == SIMPLE_FONT_BACKEND_ASCII);
    const simple_font_glyph *g = simple_font_get_glyph(&font, 'A');
    mu_check(g != NULL);
    mu_assert_int_eq(SIMPLE_FONT_GLYPH_SIZE, g->width);
}

MU_TEST(test_load_valid_ttf_path_uses_ttf_backend) {
    simple_font font = simple_font_load(TEST_TTF_PATH);

    mu_check(font.backend == SIMPLE_FONT_BACKEND_TTF);
    simple_font_atlas atlas = simple_font_get_atlas(&font);
    mu_assert_int_eq(SIMPLE_FONT_TTF_ATLAS_WIDTH, atlas.width);
    mu_assert_int_eq(SIMPLE_FONT_TTF_ATLAS_HEIGHT, atlas.height);
}

MU_TEST(test_ttf_glyph_produces_nonempty_bitmap) {
    simple_font font = simple_font_load(TEST_TTF_PATH);
    mu_check(font.backend == SIMPLE_FONT_BACKEND_TTF);

    const simple_font_glyph *g = simple_font_get_glyph(&font, 'A');
    mu_check(g != NULL);
    mu_check(g->width > 0);
    mu_check(g->height > 0);

    simple_font_atlas atlas = simple_font_get_atlas(&font);
    bool              lit   = false;
    for (int py = 0; py < g->height && !lit; py++)
        for (int px = 0; px < g->width; px++)
            if (atlas_pixel(&atlas, g, px, py) != 0) {
                lit = true;
                break;
            }
    mu_check(lit);
}

MU_TEST(test_glyph_accessor_out_of_range_returns_null) {
    simple_font font = simple_font_load(NULL);

    mu_check(simple_font_get_glyph(&font, '\x01') == NULL);
    mu_check(simple_font_get_glyph(&font, (char)127) == NULL);
}

MU_TEST(test_glyph_accessor_dimensions) {
    simple_font font = simple_font_load(NULL);

    const simple_font_glyph *g = simple_font_get_glyph(&font, 'A');
    mu_check(g != NULL);
    mu_assert_int_eq(SIMPLE_FONT_GLYPH_SIZE, g->width);
    mu_assert_int_eq(SIMPLE_FONT_GLYPH_SIZE, g->height);
}

MU_TEST(test_glyph_atlas_coordinates_match_grid_layout) {
    simple_font font = simple_font_load(NULL);

    // 'A' is glyph index 65-32=33 -> col 33%16=1, row 33/16=2
    const simple_font_glyph *g = simple_font_get_glyph(&font, 'A');
    mu_check(g != NULL);
    mu_assert_int_eq(1 * SIMPLE_FONT_GLYPH_SIZE, g->x);
    mu_assert_int_eq(2 * SIMPLE_FONT_GLYPH_SIZE, g->y);
}

MU_TEST(test_space_glyph_is_blank) {
    simple_font font = simple_font_load(NULL);

    const simple_font_glyph *g     = simple_font_get_glyph(&font, ' ');
    simple_font_atlas        atlas = simple_font_get_atlas(&font);
    mu_check(g != NULL);
    for (int py = 0; py < SIMPLE_FONT_GLYPH_SIZE; py++)
        for (int px = 0; px < SIMPLE_FONT_GLYPH_SIZE; px++)
            mu_assert_int_eq(0, atlas_pixel(&atlas, g, px, py));
}

MU_TEST(test_glyph_bit_pattern_matches_font8x8_basic) {
    simple_font font = simple_font_load(NULL);

    // 'A' row 0 = 0x0C (00001100) -> pixels 2,3 lit, rest dark
    const simple_font_glyph *g     = simple_font_get_glyph(&font, 'A');
    simple_font_atlas        atlas = simple_font_get_atlas(&font);
    mu_check(g != NULL);
    mu_assert_int_eq(0, atlas_pixel(&atlas, g, 0, 0));
    mu_assert_int_eq(0, atlas_pixel(&atlas, g, 1, 0));
    mu_assert_int_eq(255, atlas_pixel(&atlas, g, 2, 0));
    mu_assert_int_eq(255, atlas_pixel(&atlas, g, 3, 0));
    mu_assert_int_eq(0, atlas_pixel(&atlas, g, 4, 0));
}

MU_TEST_SUITE(simple_font_suite) {
    MU_RUN_TEST(test_load_null_path_fills_ascii_backend);
    MU_RUN_TEST(test_load_nonexistent_path_falls_back_to_ascii);
    MU_RUN_TEST(test_load_valid_ttf_path_uses_ttf_backend);
    MU_RUN_TEST(test_ttf_glyph_produces_nonempty_bitmap);
    MU_RUN_TEST(test_glyph_accessor_out_of_range_returns_null);
    MU_RUN_TEST(test_glyph_accessor_dimensions);
    MU_RUN_TEST(test_glyph_atlas_coordinates_match_grid_layout);
    MU_RUN_TEST(test_space_glyph_is_blank);
    MU_RUN_TEST(test_glyph_bit_pattern_matches_font8x8_basic);
}

int main(void) {
    MU_RUN_SUITE(simple_font_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
