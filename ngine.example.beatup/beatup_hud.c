#include "beatup_hud.h"

const char *BEATUP_JUDGE_PATHS[5] = {
     BEATUP_ASSET_DIR "perfect.png", BEATUP_ASSET_DIR "great.png",
     BEATUP_ASSET_DIR "cool.png",    BEATUP_ASSET_DIR "bad.png",
     BEATUP_ASSET_DIR "miss.png",
};

const char *BEATUP_LETTER_PATHS[6] = {
     BEATUP_ASSET_DIR "space_frame_letter_b.png",
     BEATUP_ASSET_DIR "space_frame_letter_e.png",
     BEATUP_ASSET_DIR "space_frame_letter_a.png",
     BEATUP_ASSET_DIR "space_frame_letter_t.png",
     BEATUP_ASSET_DIR "space_frame_letter_u.png",
     BEATUP_ASSET_DIR "space_frame_letter_p.png",
};

const char *BEATUP_LANE_PATHS[6] = {
     BEATUP_ASSET_DIR "a71.png", BEATUP_ASSET_DIR "a41.png", BEATUP_ASSET_DIR "a11.png",
     BEATUP_ASSET_DIR "a91.png", BEATUP_ASSET_DIR "a61.png", BEATUP_ASSET_DIR "a31.png",
};

const char *BEATUP_LANE_HIT_PATHS[6] = {
     BEATUP_ASSET_DIR "a75.png", BEATUP_ASSET_DIR "a45.png", BEATUP_ASSET_DIR "a15.png",
     BEATUP_ASSET_DIR "a95.png", BEATUP_ASSET_DIR "a65.png", BEATUP_ASSET_DIR "a35.png",
};

const char *BEATUP_LANE_DOWN_PATHS[6] = {
     BEATUP_ASSET_DIR "lane_7.png", BEATUP_ASSET_DIR "lane_4.png",
     BEATUP_ASSET_DIR "lane_1.png", BEATUP_ASSET_DIR "lane_9.png",
     BEATUP_ASSET_DIR "lane_6.png", BEATUP_ASSET_DIR "lane_3.png",
};

static float beatup_canvas_w = 1600.0f;
static float beatup_canvas_h = 900.0f;

static sprite_texture beatup_lane_l, beatup_lane_r;
static sprite_texture beatup_landing_l, beatup_landing_r;
static sprite_texture beatup_table_l, beatup_table_r;
static sprite_texture beatup_marker;
static sprite_texture beatup_space_frame;
static sprite_texture beatup_space_explode;
static sprite_texture beatup_space_hit_explode;
static sprite_texture beatup_arrow_explode;

static sprite_texture beatup_letter_tex[6];
static sprite_texture beatup_letter_glow_blue;
static sprite_texture beatup_letter_glow_yellow;
static sprite_texture beatup_frame_glow_blue;
static sprite_texture beatup_frame_glow_yellow;

static sprite_texture beatup_judge_tex[5];

typedef struct {
    float col_top;
    float judge_y;
    float space_frame_x, space_frame_y, space_frame_w, space_frame_h;
    float cursor_y, space_cursor_base_x;
    float row_yofs[3];
    float dt;
} beatup_frame_layout;

static float beatup_lane_x(bool right_side) {
    if (!right_side) return BEATUP_TABLE_W - BEATUP_TABLE_TR;
    return beatup_canvas_w - BEATUP_TABLE_W + BEATUP_TABLE_TR - BEATUP_LANE_W;
}

static float beatup_landing_x(bool right_side) {
    float lane_x = beatup_lane_x(right_side);
    return right_side ? lane_x - BEATUP_LANDING_W : lane_x + BEATUP_LANE_W;
}

float beatup_note_x_at(bool right_col, float time_to_hit_ms) {
    float landing_center = beatup_landing_x(right_col) + BEATUP_LANDING_W * 0.5f;
    float dir            = right_col ? 1.0f : -1.0f;
    return landing_center + dir * time_to_hit_ms * BEATUP_NOTE_SPEED_PX_PER_MS;
}

static float beatup_arrow_explode_center_x(bool right_col) {
    float near_edge =
         right_col ? beatup_landing_x(true) + BEATUP_LANDING_W : beatup_landing_x(false);
    float ofs = BEATUP_NOTE_SIZE * 0.5f + BEATUP_ARROW_LANE_OFS;
    return right_col ? near_edge - ofs : near_edge + ofs;
}

static void beatup_letters_draw_range(sprite_texture glow, int first, int last,
                                      float center_x, float center_y) {
    float glow_w = (float)glow.width * BEATUP_SCALE;
    float glow_h = (float)glow.height * BEATUP_SCALE;
    for (int i = first; i < last; i++) {
        float letter_x = center_x - BEATUP_LETTER_DIST * 0.5f * (5.0f - (float)i * 2.0f);
        render_sprite_draw(glow, letter_x - glow_w * 0.5f, center_y - glow_h * 0.5f,
                           glow_w, glow_h, COLOR4F_WHITE);

        float letter_w = (float)beatup_letter_tex[i].width * BEATUP_SCALE;
        float letter_h = (float)beatup_letter_tex[i].height * BEATUP_SCALE;
        render_sprite_draw(beatup_letter_tex[i], letter_x - letter_w * 0.5f,
                           center_y - letter_h * 0.5f, letter_w, letter_h, COLOR4F_WHITE);
    }
}

static void beatup_letters_draw(float center_x, float center_y) {
    int num_blue = 0, num_yellow = 0;

    if (beatup_combo >= 400) {
        num_blue = 6;
    } else if (beatup_combo >= 100) {
        num_blue   = (beatup_combo - 100) / 50;
        num_yellow = 6 - num_blue;
    } else if (beatup_combo >= 80) {
        num_yellow = 5;
    } else if (beatup_combo >= 60) {
        num_yellow = 4;
    } else if (beatup_combo >= 40) {
        num_yellow = 3;
    } else if (beatup_combo >= 20) {
        num_yellow = 2;
    } else if (beatup_combo >= 10) {
        num_yellow = 1;
    }

    beatup_letters_draw_range(beatup_letter_glow_blue, 0, num_blue, center_x, center_y);
    beatup_letters_draw_range(beatup_letter_glow_yellow, num_blue, num_blue + num_yellow,
                              center_x, center_y);
}

static float beatup_text_width(const simple_font *font, const char *str, float scale) {
    float width = 0.0f;
    for (const char *p = str; *p; p++) {
        const simple_font_glyph *g = simple_font_get_glyph(font, *p);
        width +=
             g ? (float)g->advance * scale : (float)simple_font_get_advance(font) * scale;
    }
    return width;
}

static event_callback_result beatup_on_resize(event_context *ctx, void *userdata) {
    (void)userdata;
    const sys_event_resize_payload *payload = ctx->payload;
    beatup_canvas_w                         = (float)payload->width;
    beatup_canvas_h                         = (float)payload->height;
    return (event_callback_result){.type = EVENT_CALLBACK_RESULT_VOID};
}

void beatup_layout_init(void) {
    beatup_canvas_w = (float)zngine_window_width();
    beatup_canvas_h = (float)zngine_window_height();
    zngine_event_subscribe(EVENT_CATEGORY_SYSTEM, SYS_EVENT_ON_RESIZE_WINDOW,
                           beatup_on_resize, NULL, NULL);

    beatup_lane_l      = sprite_texture_load(BEATUP_ASSET_DIR "laneL.png");
    beatup_lane_r      = sprite_texture_load(BEATUP_ASSET_DIR "laneR.png");
    beatup_landing_l   = sprite_texture_load(BEATUP_ASSET_DIR "landingL.png");
    beatup_landing_r   = sprite_texture_load(BEATUP_ASSET_DIR "landingR.png");
    beatup_table_l     = sprite_texture_load(BEATUP_ASSET_DIR "tableL.png");
    beatup_table_r     = sprite_texture_load(BEATUP_ASSET_DIR "tableR.png");
    beatup_space_frame = sprite_texture_load(BEATUP_ASSET_DIR "space_frame.png");
    beatup_space_explode =
         sprite_texture_load(BEATUP_ASSET_DIR "space_frame_explode.png");
    beatup_space_hit_explode =
         sprite_texture_load(BEATUP_ASSET_DIR "space_frame_space_explode.png");
    beatup_marker        = sprite_texture_load(BEATUP_ASSET_DIR "space_frame_cursor.png");
    beatup_arrow_explode = sprite_texture_load(BEATUP_ASSET_DIR "arrow_explode.png");
    beatup_letter_glow_blue =
         sprite_texture_load(BEATUP_ASSET_DIR "space_frame_letter_glow_blue.png");
    beatup_letter_glow_yellow =
         sprite_texture_load(BEATUP_ASSET_DIR "space_frame_letter_glow_yellow.png");
    beatup_frame_glow_blue =
         sprite_texture_load(BEATUP_ASSET_DIR "space_frame_glow_blue.png");
    beatup_frame_glow_yellow =
         sprite_texture_load(BEATUP_ASSET_DIR "space_frame_glow_yellow.png");
    for (int i = 0; i < 5; i++) {
        beatup_judge_tex[i] = sprite_texture_load(BEATUP_JUDGE_PATHS[i]);
    }
    for (int i = 0; i < 6; i++) {
        beatup_letter_tex[i] = sprite_texture_load(BEATUP_LETTER_PATHS[i]);
    }
    for (int i = 0; i < 6; i++) {
        beatup_lanes[i].tex           = sprite_texture_load(BEATUP_LANE_PATHS[i]);
        beatup_lanes[i].hit_tex       = sprite_texture_load(BEATUP_LANE_HIT_PATHS[i]);
        beatup_lanes[i].lane_down_tex = sprite_texture_load(BEATUP_LANE_DOWN_PATHS[i]);
    }
}

static beatup_frame_layout beatup_layout_compute(void) {
    float content_h   = 492.0f;
    float content_top = (beatup_canvas_h - content_h) * 0.5f;

    beatup_frame_layout l = {
         .col_top       = content_top + 92.5f,
         .judge_y       = content_top - 30.0f,
         .space_frame_w = 380.0f * BEATUP_SCALE,
         .space_frame_h = 128.0f * BEATUP_SCALE,
         .space_frame_y = content_top + 300.0f,
         .row_yofs      = {3.0f, 67.0f, 131.0f},
         .dt            = zngine_clock_dt(),
    };
    l.space_frame_x       = (beatup_canvas_w - l.space_frame_w) / 2.0f;
    l.cursor_y            = l.space_frame_y + (l.space_frame_h - BEATUP_MARKER_H) / 2.0f;
    l.space_cursor_base_x = (beatup_canvas_w - BEATUP_MARKER_W) / 2.0f;
    return l;
}

static void beatup_draw_lanes(const beatup_frame_layout *l) {
    float table_w = (float)beatup_table_l.width * BEATUP_SCALE;
    float table_h = (float)beatup_table_l.height * BEATUP_SCALE;
    float lane_l  = beatup_lane_x(false);
    float lane_r  = beatup_lane_x(true);

    render_sprite_draw(beatup_table_l, 0.0f, l->col_top, table_w, table_h, COLOR4F_WHITE);
    render_sprite_draw(beatup_table_r, beatup_canvas_w - table_w, l->col_top, table_w,
                       table_h, COLOR4F_WHITE);

    render_sprite_draw(beatup_lane_l, lane_l, l->col_top, BEATUP_LANE_W, BEATUP_COL_H,
                       COLOR4F_WHITE);
    render_sprite_draw(beatup_lane_r, lane_r, l->col_top, BEATUP_LANE_W, BEATUP_COL_H,
                       COLOR4F_WHITE);

    float landing_l = beatup_landing_x(false);
    float landing_r = beatup_landing_x(true);
    render_sprite_draw(beatup_landing_l, landing_l, l->col_top, BEATUP_LANDING_W,
                       BEATUP_COL_H, COLOR4F_WHITE);
    render_sprite_draw(beatup_landing_r, landing_r, l->col_top, BEATUP_LANDING_W,
                       BEATUP_COL_H, COLOR4F_WHITE);
}

static void beatup_draw_space_frame(const beatup_frame_layout *l) {
    if (beatup_combo >= 400) {
        render_sprite_draw(beatup_frame_glow_blue, l->space_frame_x, l->space_frame_y,
                           l->space_frame_w, l->space_frame_h, COLOR4F_WHITE);
    } else if (beatup_combo >= 100) {
        render_sprite_draw(beatup_frame_glow_yellow, l->space_frame_x, l->space_frame_y,
                           l->space_frame_w, l->space_frame_h, COLOR4F_WHITE);
    }
    render_sprite_draw(beatup_space_frame, l->space_frame_x, l->space_frame_y,
                       l->space_frame_w, l->space_frame_h, COLOR4F_WHITE);
    beatup_letters_draw(beatup_canvas_w * 0.5f,
                        l->space_frame_y + l->space_frame_h * 0.5f);

    if (beatup_space_explode_active) {
        beatup_space_explode_timer += l->dt;
        if (beatup_space_explode_timer >= BEATUP_ARROW_ANIMATION_MS) {
            beatup_space_explode_active = false;
        } else {
            render_sprite_draw(beatup_space_explode, l->space_frame_x, l->space_frame_y,
                               l->space_frame_w, l->space_frame_h, COLOR4F_WHITE);
        }
    }

    if (beatup_space_hit_explode_active) {
        beatup_space_hit_explode_timer += l->dt;
        if (beatup_space_hit_explode_timer >= BEATUP_ARROW_ANIMATION_MS) {
            beatup_space_hit_explode_active = false;
        } else {
            render_sprite_draw(beatup_space_hit_explode, l->space_frame_x,
                               l->space_frame_y, l->space_frame_w, l->space_frame_h,
                               COLOR4F_WHITE);
        }
    }
}

static void beatup_draw_notes(const beatup_frame_layout *l) {
    int last = beatup_first_avail_note + BEATUP_LOOKAHEAD_NOTES;
    if (last > beatup_chart_count) last = beatup_chart_count;

    for (int i = beatup_first_avail_note; i < last; i++) {
        const beatup_chart_note *note = &beatup_chart[i];
        if (note->pressed) continue;

        float time_to_hit = note->time_ms - beatup_song_time_ms;
        if (time_to_hit < -BEATUP_MISS_LATE_MS) continue;
        if (time_to_hit > BEATUP_ARROW_VISIBLE_MS) break;

        if (note->key == 5) {
            if (time_to_hit > BEATUP_SPACE_VISIBLE_MS) continue;
            float ofs = time_to_hit * BEATUP_SPACE_CURSOR_SPEED_PX_PER_MS;
            render_sprite_draw(beatup_marker, l->space_cursor_base_x - ofs, l->cursor_y,
                               BEATUP_MARKER_W, BEATUP_MARKER_H, COLOR4F_WHITE);
            render_sprite_draw(beatup_marker, l->space_cursor_base_x + ofs, l->cursor_y,
                               BEATUP_MARKER_W, BEATUP_MARKER_H, COLOR4F_WHITE);
            continue;
        }

        if (time_to_hit > BEATUP_NOTE_TRAVEL_MS) continue;
        int slot = beatup_lane_slot(note->key);
        if (slot < 0) continue;
        const beatup_lane *lane  = &beatup_lanes[slot];
        float              row_y = l->col_top + l->row_yofs[lane->row] * BEATUP_SCALE;
        float              x =
             beatup_note_x_at(lane->right_col, time_to_hit) - BEATUP_NOTE_SIZE * 0.5f;
        render_sprite_draw(lane->tex, x, row_y, BEATUP_NOTE_SIZE, BEATUP_NOTE_SIZE,
                           COLOR4F_WHITE);
    }
}

static void beatup_draw_lane_overlays(const beatup_frame_layout *l) {
    for (int i = 0; i < 6; i++) {
        bool  right        = beatup_lanes[i].right_col;
        float row_y        = l->col_top + l->row_yofs[beatup_lanes[i].row] * BEATUP_SCALE;
        float row_center_y = row_y + BEATUP_NOTE_SIZE * 0.5f;

        if (beatup_lanes[i].flash_pending) {
            beatup_lanes[i].flash_pending = false;
            render_sprite_draw(beatup_lanes[i].hit_tex,
                               beatup_lanes[i].flash_x - BEATUP_NOTE_SIZE * 0.5f, row_y,
                               BEATUP_NOTE_SIZE, BEATUP_NOTE_SIZE, COLOR4F_WHITE);
        }

        if (beatup_lanes[i].lane_down_active) {
            beatup_lanes[i].lane_down_timer += l->dt;
            if (beatup_lanes[i].lane_down_timer >= BEATUP_ARROW_ANIMATION_MS) {
                beatup_lanes[i].lane_down_active = false;
            } else {
                float w = (float)beatup_lanes[i].lane_down_tex.width * BEATUP_SCALE -
                          (right ? 15.0f : 17.0f) * BEATUP_SCALE;
                float h = (float)beatup_lanes[i].lane_down_tex.height * BEATUP_SCALE;
                float x = beatup_lane_x(right) - (right ? 55.0f * BEATUP_SCALE : 0.0f);
                render_sprite_draw(beatup_lanes[i].lane_down_tex, x,
                                   row_center_y - h * 0.5f, w, h, COLOR4F_WHITE);
            }
        }

        if (beatup_lanes[i].overlay_active) {
            beatup_lanes[i].overlay_timer += l->dt;
            if (beatup_lanes[i].overlay_timer >= BEATUP_ARROW_ANIMATION_MS) {
                beatup_lanes[i].overlay_active = false;
            } else {
                float w = (float)beatup_arrow_explode.width * BEATUP_SCALE;
                float h = (float)beatup_arrow_explode.height * BEATUP_SCALE;
                render_sprite_draw(beatup_arrow_explode,
                                   beatup_arrow_explode_center_x(right) - w * 0.5f,
                                   row_center_y - h * 0.5f, w, h, COLOR4F_WHITE);
            }
        }
    }
}

static void beatup_draw_judge_popup(const beatup_frame_layout *l) {
    if (!beatup_judge_active) return;

    beatup_judge_timer += l->dt;
    if (beatup_judge_timer >= BEATUP_JUDGE_HOLD) beatup_judge_active = false;

    float ratio = 1.0f;
    if (beatup_judge_timer < 0.05f) ratio = 1.0f + (0.05f - beatup_judge_timer) / 0.09f;

    float judge_w        = BEATUP_JUDGE_W * ratio;
    float judge_h        = BEATUP_JUDGE_H * ratio;
    float judge_center_x = beatup_canvas_w * 0.5f;
    float judge_center_y = l->judge_y + BEATUP_JUDGE_H * 0.5f;
    render_sprite_draw(beatup_judge_tex[beatup_judge_index],
                       judge_center_x - judge_w * 0.5f, judge_center_y - judge_h * 0.5f,
                       judge_w, judge_h, COLOR4F_WHITE);
}

static void beatup_draw_countdown(const beatup_frame_layout *l) {
    if (!beatup_countdown_active) return;
    // Beat-unit stages, ported from the original beatup JS: GET READY (32 beats out),
    // START (16), 3/2/1 (12/8/4), then the note's own travel-in animation takes over.
    float       remaining = beatup_first_note_time_ms - beatup_song_time_ms;
    const char *text      = remaining > 32.0f * BEATUP_TICK_TIME_MS   ? "GET READY"
                            : remaining > 16.0f * BEATUP_TICK_TIME_MS ? "START"
                            : remaining > 12.0f * BEATUP_TICK_TIME_MS ? "3"
                            : remaining > 8.0f * BEATUP_TICK_TIME_MS  ? "2"
                            : remaining > 4.0f * BEATUP_TICK_TIME_MS  ? "1"
                                                                      : NULL;
    if (!text) return;

    // Same slot the judge popup (perfect/great/cool/bad/miss) renders into.
    float judge_center_x = beatup_canvas_w * 0.5f;
    float judge_center_y = l->judge_y + BEATUP_JUDGE_H * 0.5f;

    const simple_font *font      = zngine_font_primary_get();
    float              font_size = 64.0f;
    float              scale     = font_size / (float)simple_font_get_advance(font);
    float              width     = beatup_text_width(font, text, scale);
    render_text_draw_basic(judge_center_x - width * 0.5f,
                           judge_center_y - font_size * 0.5f, text, scale, COLOR4F_WHITE,
                           font, 0.0f);
}

void beatup_layout_draw(void) {
    beatup_frame_layout l = beatup_layout_compute();
    beatup_draw_lanes(&l);
    beatup_draw_space_frame(&l);
    beatup_draw_notes(&l);
    beatup_draw_lane_overlays(&l);
    beatup_draw_judge_popup(&l);
    beatup_draw_countdown(&l);
}

void beatup_hud_draw(void) {
    float content_h       = 492.0f;
    float content_top     = (beatup_canvas_h - content_h) * 0.5f + 15.0f;
    float hud_score_y     = content_top + 110.0f;
    float hud_combo_y     = content_top + 140.0f;
    float hud_max_combo_y = content_top + 170.0f;
    float hud_perfect_y   = content_top + 200.0f;
    float hud_stats_y     = content_top + 230.0f;

    const simple_font *font      = zngine_font_primary_get();
    float              font_size = 21.0f;
    float              scale     = font_size / (float)simple_font_get_advance(font);

    int total_judged =
         beatup_judge_count[BEATUP_JUDGE_PERFECT] +
         beatup_judge_count[BEATUP_JUDGE_GREAT] + beatup_judge_count[BEATUP_JUDGE_COOL] +
         beatup_judge_count[BEATUP_JUDGE_BAD] + beatup_judge_count[BEATUP_JUDGE_MISS];
    float perfect_pct = total_judged > 0
                             ? 100.0f * (float)beatup_judge_count[BEATUP_JUDGE_PERFECT] /
                                    (float)total_judged
                             : 0.0f;

    char score_buf[32];
    char combo_buf[32];
    char max_combo_buf[32];
    char perfect_buf[32];
    char stats_buf[48];
    snprintf(score_buf, sizeof(score_buf), "Score: %d", (int)roundf(beatup_score));
    snprintf(combo_buf, sizeof(combo_buf), "Combo: %d", beatup_combo);
    snprintf(max_combo_buf, sizeof(max_combo_buf), "Max Combo: %d", beatup_max_combo);
    snprintf(perfect_buf, sizeof(perfect_buf), "Perfect: %.2f%%", perfect_pct);
    snprintf(stats_buf, sizeof(stats_buf), "P/G/C/B/M: %d/%d/%d/%d/%d",
             beatup_judge_count[BEATUP_JUDGE_PERFECT],
             beatup_judge_count[BEATUP_JUDGE_GREAT],
             beatup_judge_count[BEATUP_JUDGE_COOL], beatup_judge_count[BEATUP_JUDGE_BAD],
             beatup_judge_count[BEATUP_JUDGE_MISS]);

    float widest = beatup_text_width(font, score_buf, scale);
    widest       = fmaxf(widest, beatup_text_width(font, combo_buf, scale));
    widest       = fmaxf(widest, beatup_text_width(font, max_combo_buf, scale));
    widest       = fmaxf(widest, beatup_text_width(font, perfect_buf, scale));
    widest       = fmaxf(widest, beatup_text_width(font, stats_buf, scale));
    float panel_center_x =
         (beatup_landing_x(false) + BEATUP_LANDING_W + beatup_landing_x(true)) * 0.5f;
    float hud_x = panel_center_x - widest * 0.5f;

    render_text_draw_basic(hud_x, hud_score_y, score_buf, scale, COLOR4F_WHITE, font,
                           0.0f);
    render_text_draw_basic(hud_x, hud_combo_y, combo_buf, scale, COLOR4F_WHITE, font,
                           0.0f);
    render_text_draw_basic(hud_x, hud_max_combo_y, max_combo_buf, scale, COLOR4F_WHITE,
                           font, 0.0f);
    render_text_draw_basic(hud_x, hud_perfect_y, perfect_buf, scale, COLOR4F_WHITE, font,
                           0.0f);
    render_text_draw_basic(hud_x, hud_stats_y, stats_buf, scale, COLOR4F_WHITE, font,
                           0.0f);
}

void beatup_layout_destroy(void) {
    sprite_texture_unload(&beatup_lane_l);
    sprite_texture_unload(&beatup_lane_r);
    sprite_texture_unload(&beatup_landing_l);
    sprite_texture_unload(&beatup_landing_r);
    sprite_texture_unload(&beatup_table_l);
    sprite_texture_unload(&beatup_table_r);
    sprite_texture_unload(&beatup_marker);
    sprite_texture_unload(&beatup_space_frame);
    sprite_texture_unload(&beatup_space_explode);
    sprite_texture_unload(&beatup_space_hit_explode);
    sprite_texture_unload(&beatup_arrow_explode);
    sprite_texture_unload(&beatup_letter_glow_blue);
    sprite_texture_unload(&beatup_letter_glow_yellow);
    sprite_texture_unload(&beatup_frame_glow_blue);
    sprite_texture_unload(&beatup_frame_glow_yellow);
    for (int i = 0; i < 5; i++) sprite_texture_unload(&beatup_judge_tex[i]);
    for (int i = 0; i < 6; i++) sprite_texture_unload(&beatup_lanes[i].tex);
    for (int i = 0; i < 6; i++) sprite_texture_unload(&beatup_lanes[i].hit_tex);
    for (int i = 0; i < 6; i++) sprite_texture_unload(&beatup_lanes[i].lane_down_tex);
    for (int i = 0; i < 6; i++) sprite_texture_unload(&beatup_letter_tex[i]);
}
