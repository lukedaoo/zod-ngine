#include <stdbool.h>

#define ZOD_NGINE_IMPLEMENTATION
#include <ngine.core/index.h>
#include <ngine.ext.console/index.h>

#define CONFIG_PATH "run-tree/data/engine.scf"

#define BEATUP_ASSET_DIR "run-tree/data/textures/beatup/"

static float beatup_canvas_w = 1600.0f;
static float beatup_canvas_h = 900.0f;

#define BEATUP_TABLE_W    123.0f
#define BEATUP_TABLE_TR   3.0f
#define BEATUP_LANE_W     256.0f
#define BEATUP_LANDING_W  100.0f
#define BEATUP_CHANCE_DST 80.0f

#define BEATUP_SCALE                1.0f
#define BEATUP_COL_W                (BEATUP_LANE_W * BEATUP_SCALE)
#define BEATUP_COL_H                (256.0f * BEATUP_SCALE)
#define BEATUP_NOTE_SIZE            64.0f
#define BEATUP_NOTE_OVERSHOOT       BEATUP_NOTE_SIZE
#define BEATUP_MARKER_W             128.0f
#define BEATUP_MARKER_H             64.0f
#define BEATUP_JUDGE_W              (256.0f * 1.25f)
#define BEATUP_JUDGE_H              (128.0f * 1.25f)
#define BEATUP_LETTER_SIZE          64.0f
#define BEATUP_LETTER_GAP           8.0f
#define BEATUP_LETTER_MARGIN_BOTTOM 80.0f

static sprite_texture beatup_lane_l, beatup_lane_r;
static sprite_texture beatup_landing_l, beatup_landing_r;
static sprite_texture beatup_marker;
static float          beatup_cursor_t;
#define BEATUP_CURSOR_PERIOD 2.0f
static sprite_texture beatup_space_frame;
static const char    *BEATUP_JUDGE_PATHS[5] = {
     BEATUP_ASSET_DIR "perfect.png", BEATUP_ASSET_DIR "great.png",
     BEATUP_ASSET_DIR "cool.png",    BEATUP_ASSET_DIR "bad.png",
     BEATUP_ASSET_DIR "miss.png",
};
static sprite_texture beatup_judge_tex[5];
static float          beatup_judge_timer;
static int            beatup_judge_index;
#define BEATUP_JUDGE_HOLD 0.9f

typedef struct beatup_note {
    sprite_texture tex;
    const char    *path;
    bool           right_col;
    int            row;
    float          t;
    float          phase;
} beatup_note;

#define BEATUP_NOTE_DURATION 1.2f

static beatup_note beatup_notes[6] = {
     {.path = BEATUP_ASSET_DIR "a71.png", .right_col = false, .row = 0, .phase = 0.0f},
     {.path = BEATUP_ASSET_DIR "a41.png", .right_col = false, .row = 1, .phase = 0.4f},
     {.path = BEATUP_ASSET_DIR "a11.png", .right_col = false, .row = 2, .phase = 0.8f},
     {.path = BEATUP_ASSET_DIR "a91.png", .right_col = true, .row = 0, .phase = 0.2f},
     {.path = BEATUP_ASSET_DIR "a61.png", .right_col = true, .row = 1, .phase = 0.6f},
     {.path = BEATUP_ASSET_DIR "a31.png", .right_col = true, .row = 2, .phase = 1.0f},
};

static sprite_texture beatup_letter_tex[6];

static float beatup_lane_x(bool right_side) {
    float left_x = BEATUP_TABLE_W - BEATUP_TABLE_TR - BEATUP_CHANCE_DST;
    if (!right_side) return left_x;
    return beatup_canvas_w - BEATUP_TABLE_W + BEATUP_TABLE_TR - BEATUP_LANE_W +
           BEATUP_CHANCE_DST;
}

static float beatup_landing_x(bool right_side) {
    float lane_x = beatup_lane_x(right_side);
    return right_side ? lane_x - BEATUP_LANDING_W : lane_x + BEATUP_LANE_W;
}

static void beatup_layout_init(void) {
    beatup_lane_l           = sprite_texture_load(BEATUP_ASSET_DIR "laneL.png");
    beatup_lane_l.draw_box  = true;
    beatup_lane_l.box_color = 0xFF0000FF;

    beatup_lane_r           = sprite_texture_load(BEATUP_ASSET_DIR "laneR.png");
    beatup_lane_r.draw_box  = true;
    beatup_lane_r.box_color = 0xFF0000FF;

    beatup_landing_l           = sprite_texture_load(BEATUP_ASSET_DIR "landingL.png");
    beatup_landing_l.draw_box  = true;
    beatup_landing_l.box_color = 0xFF0000FF;

    beatup_landing_r   = sprite_texture_load(BEATUP_ASSET_DIR "landingR.png");
    beatup_space_frame = sprite_texture_load(BEATUP_ASSET_DIR "space_frame.png");
    beatup_marker      = sprite_texture_load(BEATUP_ASSET_DIR "space_frame_cursor.png");
    for (int i = 0; i < 5; i++)
        beatup_judge_tex[i] = sprite_texture_load(BEATUP_JUDGE_PATHS[i]);

    for (int i = 0; i < 6; i++) {
        beatup_notes[i].tex           = sprite_texture_load(beatup_notes[i].path);
        beatup_notes[i].tex.draw_box  = true;
        beatup_notes[i].tex.box_color = 0xFF0000FF;
    }
    // for (int i = 0; i < 6; i++)
    //     beatup_letter_tex[i] = sprite_texture_load(BEATUP_LETTERS[i]);
}

static void beatup_layout_draw(void) {
    beatup_canvas_w = (float)zngine_window_width();
    beatup_canvas_h = (float)zngine_window_height();

    float content_h     = 492.0f;
    float content_top   = (beatup_canvas_h - content_h) * 0.5f;
    float judge_y       = content_top;
    float col_top       = content_top + 92.5f;
    float space_frame_y = content_top + 300.0f;

    float lane_l = beatup_lane_x(false);
    float lane_r = beatup_lane_x(true);

    render_sprite_draw(beatup_lane_l, lane_l, col_top, BEATUP_COL_W, BEATUP_COL_H,
                       COLOR4F_WHITE);
    render_sprite_draw(beatup_lane_r, lane_r, col_top, BEATUP_COL_W, BEATUP_COL_H,
                       COLOR4F_WHITE);

    float landing_l = beatup_landing_x(false);
    float landing_r = beatup_landing_x(true);
    render_sprite_draw(beatup_landing_l, landing_l, col_top, BEATUP_LANDING_W,
                       BEATUP_COL_H, COLOR4F_WHITE);
    render_sprite_draw(beatup_landing_r, landing_r, col_top, BEATUP_LANDING_W,
                       BEATUP_COL_H, COLOR4F_WHITE);

    static const float row_yofs[3] = {3.0f, 67.0f, 131.0f};
    float              dt          = zngine_clock_dt();

    for (int i = 0; i < 6; i++) {
        bool  right     = beatup_notes[i].right_col;
        float lane_x    = beatup_lane_x(right);
        float landing_x = beatup_landing_x(right);
        float row_y     = col_top + row_yofs[beatup_notes[i].row] * BEATUP_SCALE;

        float dir        = right ? -1.0f : 1.0f;
        float spawn_edge = right ? lane_x + BEATUP_LANE_W : lane_x;
        float far_edge   = right ? lane_x : lane_x + BEATUP_LANE_W;

        float spawn_x      = spawn_edge - (dir > 0.0f ? BEATUP_NOTE_SIZE : 0.0f);
        float travel_end_x = far_edge - (dir > 0.0f ? 0.0f : BEATUP_NOTE_SIZE) +
                             dir * BEATUP_NOTE_OVERSHOOT;

        float panel_x = right ? landing_x : landing_x + BEATUP_LANDING_W;

        beatup_notes[i].t += dt / BEATUP_NOTE_DURATION;
        float t = fmodf(beatup_notes[i].t + beatup_notes[i].phase, 1.0f);

        float note_x = spawn_x + (travel_end_x - spawn_x) * t;
        bool visible = right ? (note_x + BEATUP_NOTE_SIZE) >= panel_x : note_x <= panel_x;
        if (visible) {
            render_sprite_draw(beatup_notes[i].tex, note_x, row_y, BEATUP_NOTE_SIZE,
                               BEATUP_NOTE_SIZE, COLOR4F_WHITE);
        }
    }

    beatup_judge_timer += dt;
    if (beatup_judge_timer >= BEATUP_JUDGE_HOLD) {
        beatup_judge_timer -= BEATUP_JUDGE_HOLD;
        beatup_judge_index = (beatup_judge_index + 1) % 5;
    }
    render_sprite_draw(beatup_judge_tex[beatup_judge_index],
                       (beatup_canvas_w - BEATUP_JUDGE_W) / 2.0f, judge_y, BEATUP_JUDGE_W,
                       BEATUP_JUDGE_H, COLOR4F_WHITE);

    float space_frame_w = 380.0f * 1.5f;
    float space_frame_h = 128.0f * 1.5f;
    render_sprite_draw(beatup_space_frame, (beatup_canvas_w - space_frame_w) / 2.0f,
                       space_frame_y, space_frame_w, space_frame_h, COLOR4F_WHITE);

    float frame_x  = (beatup_canvas_w - space_frame_w) / 2.0f;
    float travel_w = space_frame_w - BEATUP_MARKER_W;
    float cursor_y = space_frame_y + (space_frame_h - BEATUP_MARKER_H) / 2.0f;
    float reach    = travel_w * 0.5f + BEATUP_MARKER_W * 0.25f;
    beatup_cursor_t += dt / (BEATUP_CURSOR_PERIOD * 0.5f);
    float t =
         fmodf(beatup_cursor_t, 1.0f);  // 0 (edges) -> 1 (past center), then snaps to 0
    render_sprite_draw(beatup_marker, frame_x + reach * t, cursor_y, BEATUP_MARKER_W,
                       BEATUP_MARKER_H, COLOR4F_WHITE);
    render_sprite_draw(beatup_marker, frame_x + travel_w - reach * t, cursor_y,
                       BEATUP_MARKER_W, BEATUP_MARKER_H, COLOR4F_WHITE);
}

static void beatup_hud_draw(void) {
    float content_h   = 492.0f;
    float content_top = (beatup_canvas_h - content_h) * 0.5f;
    float hud_score_y = content_top + 180.0f;
    float hud_combo_y = content_top + 224.0f;

    const simple_font *font      = zngine_font_primary_get();
    float              font_size = 32.0f;
    float              scale     = font_size / (float)simple_font_get_advance(font);
    float              hud_x     = beatup_canvas_w / 2.0f - 80.0f;
    render_text_draw_basic(hud_x, hud_score_y, "Score: 0", scale, COLOR4F_WHITE, font,
                           0.0f);
    render_text_draw_basic(hud_x, hud_combo_y, "Combo: 0", scale, COLOR4F_WHITE, font,
                           0.0f);
}

static void beatup_layout_destroy(void) {
    sprite_texture_unload(&beatup_lane_l);
    sprite_texture_unload(&beatup_lane_r);
    sprite_texture_unload(&beatup_landing_l);
    sprite_texture_unload(&beatup_landing_r);
    sprite_texture_unload(&beatup_marker);
    sprite_texture_unload(&beatup_space_frame);
    for (int i = 0; i < 5; i++) sprite_texture_unload(&beatup_judge_tex[i]);
    for (int i = 0; i < 6; i++) sprite_texture_unload(&beatup_notes[i].tex);
    for (int i = 0; i < 6; i++) sprite_texture_unload(&beatup_letter_tex[i]);
}

static bool load_args(const int argc, const char **argv, cvar_table *cvars) {
    carg_register defs[] = {
         {.flag = "--size", .arg_count = 2, .type = CARG_INT, .required = false},
    };

    carg_table cargs = {0};
    if (!carg_parse(defs, 1, argc, argv, &cargs)) return false;

    const char *size_names[] = {"window.width", "window.height"};
    return carg_entry_to_cvars(carg_get(&cargs, "--size"), size_names, 2, cvars);
}

int main(const int argc, const char **argv) {
    log_debug("beatup: starting");
    const zngine_dispatch dispatch = {.load_args = load_args};

    const zngine_init_params params = {
         .argc         = argc,
         .argv         = argv,
         .config_setup = {.config_path      = CONFIG_PATH,
                          .hot_reload       = true,
                          .load_config_func = load_config_from_file_default},
         .dispatch     = dispatch,
    };

#if ZOD_CONSOLE_ENABLED
    zconsole_ext_install();
#endif

    if (!zngine_init(params)) return 1;
    render_text_init();
    render_sprite_init();
    beatup_layout_init();

    while (!zngine_should_exit()) {
        zngine_clock_update();
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) zngine_request_exit();
            if (e.type == SDL_EVENT_WINDOW_RESIZED) {
                sys_event_resize_payload payload = {.width  = e.window.data1,
                                                    .height = e.window.data2};
                event_context            ctx     = {
                     .identifier   = {.category = EVENT_TAG_SYSTEM_WINDOW,
                                      .event_id = SYS_EVENT_ON_RESIZE_WINDOW},
                     .payload      = &payload,
                     .payload_size = sizeof(payload)};
                zngine_event_publish(&ctx);
            }
#if ZOD_CONSOLE_ENABLED
            zconsole_input_handle(&e);
#endif
        }
        zngine_input_update();
        zngine_tick_hot_reload();

#if ZOD_CONSOLE_ENABLED
        if (zngine_input_key_pressed(SDL_SCANCODE_GRAVE)) zconsole_toggle();
#endif

        zngine_begin_drawing();
        beatup_layout_draw();
#if ZOD_CONSOLE_ENABLED
        zconsole_draw();
#endif
        beatup_hud_draw();

        render_text_flush();
        render_sprite_flush();
        zngine_end_drawing();
        zngine_clock_sleep_to_target_fps();
    }
    beatup_layout_destroy();
    render_sprite_destroy();
    render_text_destroy();
    zngine_destroy();
    return 0;
}
