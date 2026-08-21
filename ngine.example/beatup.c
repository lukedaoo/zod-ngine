#include <stdbool.h>

#define ZOD_NGINE_IMPLEMENTATION
#include <ngine.core/index.h>
#include <ngine.ext.console/index.h>

#define CONFIG_PATH     "run-tree/data/engine_beatup.scf"
#define KEYBINDING_PATH "run-tree/data/keybind_beatup.scf"

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
#define BEATUP_COL_H                (196.0f * BEATUP_SCALE)
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

#define BEATUP_BPM           120.0f
#define BEATUP_BEAT_INTERVAL (60.0f / BEATUP_BPM)
static float          beatup_beat_timer = 0.0f;
static sprite_texture beatup_space_frame;
static sprite_texture beatup_space_explode;
static bool           beatup_space_explode_active = false;
static float          beatup_space_explode_timer  = 0.0f;
static const char    *BEATUP_JUDGE_PATHS[5]       = {
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
    sprite_texture hit_tex;
    const char    *hit_path;
    bool           right_col;
    int            row;
    float          t;
    float          phase;
    float          prev_local_t;
    bool           judged;
    bool           despawned;
    bool           hit_flash_pending;
} beatup_note;

#define BEATUP_NOTE_DURATION  1.2f
#define BEATUP_HIT_FLASH_HOLD 0.3f

#define BEATUP_JUDGE_PERFECT_PX 15.0f
#define BEATUP_JUDGE_GREAT_PX   35.0f
#define BEATUP_JUDGE_COOL_PX    55.0f
#define BEATUP_JUDGE_BAD_PX     80.0f

#define BEATUP_SCORE_PERFECT 100
#define BEATUP_SCORE_GREAT   75
#define BEATUP_SCORE_COOL    50
#define BEATUP_SCORE_BAD     25

enum : uint8_t {
    BEATUP_JUDGE_PERFECT,
    BEATUP_JUDGE_GREAT,
    BEATUP_JUDGE_COOL,
    BEATUP_JUDGE_BAD,
    BEATUP_JUDGE_MISS,
} beatup_judge_tier;

static int  beatup_score        = 0;
static int  beatup_combo        = 0;
static bool beatup_judge_active = false;

static bool  beatup_space_judged = false;
static float beatup_space_prev_t = 0.0f;

static int beatup_classify_delta(float delta_px) {
    delta_px = fabsf(delta_px);
    if (delta_px <= BEATUP_JUDGE_PERFECT_PX) return BEATUP_JUDGE_PERFECT;
    if (delta_px <= BEATUP_JUDGE_GREAT_PX) return BEATUP_JUDGE_GREAT;
    if (delta_px <= BEATUP_JUDGE_COOL_PX) return BEATUP_JUDGE_COOL;
    if (delta_px <= BEATUP_JUDGE_BAD_PX) return BEATUP_JUDGE_BAD;
    return BEATUP_JUDGE_MISS;
}

static void beatup_apply_judgment(int tier) {
    static const int scores[5] = {BEATUP_SCORE_PERFECT, BEATUP_SCORE_GREAT,
                                  BEATUP_SCORE_COOL, BEATUP_SCORE_BAD, 0};
    beatup_score += scores[tier];
    if (tier == BEATUP_JUDGE_MISS) {
        beatup_combo = 0;
    } else {
        beatup_combo++;
    }
    beatup_judge_index  = tier;
    beatup_judge_timer  = 0.0f;
    beatup_judge_active = true;
}

enum : uint8_t { INPUT_CONTEXT_GAME = 0, INPUT_CONTEXT_CONSOLE = 1 } input_contexts;

static beatup_note beatup_notes[6] = {
     {.path      = BEATUP_ASSET_DIR "a71.png",
      .hit_path  = BEATUP_ASSET_DIR "a75.png",
      .right_col = false,
      .row       = 0,
      .phase     = 0.0f},
     {.path      = BEATUP_ASSET_DIR "a41.png",
      .hit_path  = BEATUP_ASSET_DIR "a45.png",
      .right_col = false,
      .row       = 1,
      .phase     = 0.4f},
     {.path      = BEATUP_ASSET_DIR "a11.png",
      .hit_path  = BEATUP_ASSET_DIR "a15.png",
      .right_col = false,
      .row       = 2,
      .phase     = 0.8f},
     {.path      = BEATUP_ASSET_DIR "a91.png",
      .hit_path  = BEATUP_ASSET_DIR "a95.png",
      .right_col = true,
      .row       = 0,
      .phase     = 0.2f},
     {.path      = BEATUP_ASSET_DIR "a61.png",
      .hit_path  = BEATUP_ASSET_DIR "a65.png",
      .right_col = true,
      .row       = 1,
      .phase     = 0.6f},
     {.path      = BEATUP_ASSET_DIR "a31.png",
      .hit_path  = BEATUP_ASSET_DIR "a35.png",
      .right_col = true,
      .row       = 2,
      .phase     = 1.0f},
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

static float beatup_note_center_x(int i, float *out_landing_center_x) {
    const beatup_note *n         = &beatup_notes[i];
    bool               right     = n->right_col;
    float              lane_x    = beatup_lane_x(right);
    float              landing_x = beatup_landing_x(right);

    float dir        = right ? -1.0f : 1.0f;
    float spawn_edge = right ? lane_x + BEATUP_LANE_W : lane_x;
    float far_edge   = right ? lane_x : lane_x + BEATUP_LANE_W;

    float spawn_x = spawn_edge - (dir > 0.0f ? BEATUP_NOTE_SIZE : 0.0f);
    float travel_end_x =
         far_edge - (dir > 0.0f ? 0.0f : BEATUP_NOTE_SIZE) + dir * BEATUP_NOTE_OVERSHOOT;

    float t      = fmodf(n->t + n->phase, 1.0f);
    float note_x = spawn_x + (travel_end_x - spawn_x) * t;

    if (out_landing_center_x) *out_landing_center_x = landing_x + BEATUP_LANDING_W * 0.5f;
    return note_x + BEATUP_NOTE_SIZE * 0.5f;
}

static float beatup_space_gap_px(void) {
    float space_frame_w = 380.0f * 1.5f;
    float travel_w      = space_frame_w - BEATUP_MARKER_W;
    float reach         = travel_w * 0.5f + BEATUP_MARKER_W * 0.25f;
    float t             = fmodf(beatup_cursor_t, 1.0f);
    return travel_w - 2.0f * reach * t;
}

static void beatup_layout_init(void) {
    beatup_lane_l          = sprite_texture_load(BEATUP_ASSET_DIR "laneL.png");
    beatup_lane_l.draw_box = true;

    beatup_lane_r          = sprite_texture_load(BEATUP_ASSET_DIR "laneR.png");
    beatup_lane_r.draw_box = true;

    beatup_landing_l          = sprite_texture_load(BEATUP_ASSET_DIR "landingL.png");
    beatup_landing_l.draw_box = true;

    beatup_landing_r            = sprite_texture_load(BEATUP_ASSET_DIR "landingR.png");
    beatup_landing_r.draw_box   = true;
    beatup_space_frame          = sprite_texture_load(BEATUP_ASSET_DIR "space_frame.png");
    beatup_space_frame.draw_box = true;
    beatup_space_explode =
         sprite_texture_load(BEATUP_ASSET_DIR "space_frame_explode.png");
    beatup_space_explode.draw_box = true;
    beatup_marker = sprite_texture_load(BEATUP_ASSET_DIR "space_frame_cursor.png");
    beatup_marker.draw_box = true;
    for (int i = 0; i < 5; i++) {
        beatup_judge_tex[i]          = sprite_texture_load(BEATUP_JUDGE_PATHS[i]);
        beatup_judge_tex[i].draw_box = true;
    }

    for (int i = 0; i < 6; i++) {
        beatup_notes[i].tex              = sprite_texture_load(beatup_notes[i].path);
        beatup_notes[i].tex.draw_box     = true;
        beatup_notes[i].hit_tex          = sprite_texture_load(beatup_notes[i].hit_path);
        beatup_notes[i].hit_tex.draw_box = true;
    }
    // for (int i = 0; i < 6; i++)/home/luked/Downloads/a15.png

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

    beatup_beat_timer += dt;
    bool beat_tick = beatup_beat_timer >= BEATUP_BEAT_INTERVAL;
    if (beat_tick) beatup_beat_timer -= BEATUP_BEAT_INTERVAL;

    for (int i = 0; i < 6; i++) {
        if (beatup_notes[i].despawned) {
            if (beatup_notes[i].hit_flash_pending) {
                beatup_notes[i].hit_flash_pending = false;
                float note_center_x               = beatup_note_center_x(i, NULL);
                float row_y = col_top + row_yofs[beatup_notes[i].row] * BEATUP_SCALE;
                render_sprite_draw(beatup_notes[i].hit_tex,
                                   note_center_x - BEATUP_NOTE_SIZE * 0.5f, row_y,
                                   BEATUP_NOTE_SIZE, BEATUP_NOTE_SIZE, COLOR4F_WHITE);
            }
            if (!beat_tick) continue;
            beatup_notes[i].t            = -beatup_notes[i].phase;
            beatup_notes[i].prev_local_t = 0.0f;
            beatup_notes[i].judged       = false;
            beatup_notes[i].despawned    = false;
        }

        bool  right     = beatup_notes[i].right_col;
        float landing_x = beatup_landing_x(right);
        float row_y     = col_top + row_yofs[beatup_notes[i].row] * BEATUP_SCALE;
        float panel_x   = right ? landing_x : landing_x + BEATUP_LANDING_W;

        beatup_notes[i].t += dt / BEATUP_NOTE_DURATION;

        float local_t = fmodf(beatup_notes[i].t + beatup_notes[i].phase, 1.0f);
        if (local_t < beatup_notes[i].prev_local_t) {
            if (!beatup_notes[i].judged) beatup_apply_judgment(BEATUP_JUDGE_MISS);
            beatup_notes[i].judged = false;
        }
        beatup_notes[i].prev_local_t = local_t;

        float note_x = beatup_note_center_x(i, NULL) - BEATUP_NOTE_SIZE * 0.5f;
        bool visible = right ? (note_x + BEATUP_NOTE_SIZE) >= panel_x : note_x <= panel_x;
        if (visible) {
            render_sprite_draw(beatup_notes[i].tex, note_x, row_y, BEATUP_NOTE_SIZE,
                               BEATUP_NOTE_SIZE, COLOR4F_WHITE);
        }
    }

    if (beatup_judge_active) {
        beatup_judge_timer += dt;
        if (beatup_judge_timer >= BEATUP_JUDGE_HOLD) beatup_judge_active = false;
        render_sprite_draw(beatup_judge_tex[beatup_judge_index],
                           (beatup_canvas_w - BEATUP_JUDGE_W) / 2.0f, judge_y,
                           BEATUP_JUDGE_W, BEATUP_JUDGE_H, COLOR4F_WHITE);
    }

    float space_frame_w = 380.0f * 1.5f;
    float space_frame_h = 128.0f * 1.5f;
    render_sprite_draw(beatup_space_frame, (beatup_canvas_w - space_frame_w) / 2.0f,
                       space_frame_y, space_frame_w, space_frame_h, COLOR4F_WHITE);

    if (beatup_space_explode_active) {
        beatup_space_explode_timer += dt;
        if (beatup_space_explode_timer >= BEATUP_HIT_FLASH_HOLD) {
            beatup_space_explode_active = false;
        } else {
            render_sprite_draw(beatup_space_explode,
                               (beatup_canvas_w - space_frame_w) / 2.0f, space_frame_y,
                               space_frame_w, space_frame_h, COLOR4F_WHITE);
        }
    }

    float frame_x  = (beatup_canvas_w - space_frame_w) / 2.0f;
    float travel_w = space_frame_w - BEATUP_MARKER_W;
    float cursor_y = space_frame_y + (space_frame_h - BEATUP_MARKER_H) / 2.0f;
    float reach    = travel_w * 0.5f + BEATUP_MARKER_W * 0.25f;
    beatup_cursor_t += dt / (BEATUP_CURSOR_PERIOD * 0.5f);
    float t =
         fmodf(beatup_cursor_t, 1.0f);  // 0 (edges) -> 1 (past center), then snaps to 0
    if (t < beatup_space_prev_t) {
        if (!beatup_space_judged) beatup_apply_judgment(BEATUP_JUDGE_MISS);
        beatup_space_judged = false;
    }
    beatup_space_prev_t = t;
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

    char score_buf[32];
    char combo_buf[32];
    snprintf(score_buf, sizeof(score_buf), "Score: %d", beatup_score);
    snprintf(combo_buf, sizeof(combo_buf), "Combo: %d", beatup_combo);
    render_text_draw_basic(hud_x, hud_score_y, score_buf, scale, COLOR4F_WHITE, font,
                           0.0f);
    render_text_draw_basic(hud_x, hud_combo_y, combo_buf, scale, COLOR4F_WHITE, font,
                           0.0f);
}

static void beatup_layout_destroy(void) {
    sprite_texture_unload(&beatup_lane_l);
    sprite_texture_unload(&beatup_lane_r);
    sprite_texture_unload(&beatup_landing_l);
    sprite_texture_unload(&beatup_landing_r);
    sprite_texture_unload(&beatup_marker);
    sprite_texture_unload(&beatup_space_frame);
    sprite_texture_unload(&beatup_space_explode);
    for (int i = 0; i < 5; i++) sprite_texture_unload(&beatup_judge_tex[i]);
    for (int i = 0; i < 6; i++) sprite_texture_unload(&beatup_notes[i].tex);
    for (int i = 0; i < 6; i++) sprite_texture_unload(&beatup_notes[i].hit_tex);
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

#if ZOD_CONSOLE_ENABLED
static action_execute_result toggle_console_execute(const action_table *table,
                                                    action_handle self, void *userdata) {
    (void)table;
    (void)self;
    (void)userdata;
    zconsole_toggle();
    return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
}
#endif

static action_execute_result toggle_pause_execute(const action_table *table,
                                                  action_handle self, void *userdata) {
    (void)table;
    (void)self;
    (void)userdata;
    g_ctx.clock.paused = !g_ctx.clock.paused;
    return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
}

// index into beatup_notes — R/F/V = left col rows 0/1/2, I/K/M = right col
// rows 0/1/2, matching beatup_notes' fixed layout order.
static const int BEATUP_LANE_KEYS[6] = {
     SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V,
     SDL_SCANCODE_I, SDL_SCANCODE_K, SDL_SCANCODE_M,
};

static action_execute_result beatup_lane_hit_execute(const action_table *table,
                                                     action_handle self, void *userdata) {
    (void)userdata;
    action_info info;
    if (!action_lookup(table, self, &info))
        return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};

    int note_index = -1;
    for (int i = 0; i < 6; i++) {
        if (BEATUP_LANE_KEYS[i] == info.key) {
            note_index = i;
            break;
        }
    }
    if (note_index < 0)
        return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};

    beatup_note *n = &beatup_notes[note_index];
    if (n->despawned || n->judged)
        return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};

    float landing_center_x;
    float note_center_x = beatup_note_center_x(note_index, &landing_center_x);

    n->judged    = true;
    int tier     = beatup_classify_delta(note_center_x - landing_center_x);
    n->despawned = tier != BEATUP_JUDGE_MISS;
    if (n->despawned) n->hit_flash_pending = true;
    beatup_apply_judgment(tier);

    return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
}

static action_execute_result beatup_space_hit_execute(const action_table *table,
                                                      action_handle       self,
                                                      void               *userdata) {
    (void)table;
    (void)self;
    (void)userdata;
    if (beatup_space_judged)
        return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};

    beatup_space_judged = true;
    int tier            = beatup_classify_delta(beatup_space_gap_px());
    if (tier != BEATUP_JUDGE_MISS) {
        beatup_space_explode_active = true;
        beatup_space_explode_timer  = 0.0f;
    }
    beatup_apply_judgment(tier);

    return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
}

static void beatup_actions_bind(void) {
    action_executor toggle_pause = {.execute = toggle_pause_execute};
    zngine_action_bind(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED, SDL_SCANCODE_P,
                       "toggle_pause", &toggle_pause);

#if ZOD_CONSOLE_ENABLED
    action_executor toggle_console = {.execute = toggle_console_execute};
    zngine_action_bind(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED, SDL_SCANCODE_GRAVE,
                       "toggle_console", &toggle_console);
    zngine_action_bind(INPUT_CONTEXT_CONSOLE, ACTION_TRIGGER_KEY_PRESSED,
                       SDL_SCANCODE_GRAVE, "toggle_console", &toggle_console);
#endif

    action_executor lane_hit = {.execute = beatup_lane_hit_execute};
    static const struct {
        int         key;
        const char *name;
    } lane_binds[6] = {
         {SDL_SCANCODE_R, "hit_left_top"},  {SDL_SCANCODE_F, "hit_left_mid"},
         {SDL_SCANCODE_V, "hit_left_bot"},  {SDL_SCANCODE_I, "hit_right_top"},
         {SDL_SCANCODE_K, "hit_right_mid"}, {SDL_SCANCODE_M, "hit_right_bot"},
    };
    for (int i = 0; i < 6; i++) {
        zngine_action_bind(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED,
                           lane_binds[i].key, lane_binds[i].name, &lane_hit);
    }

    action_executor space_hit = {.execute = beatup_space_hit_execute};
    zngine_action_bind(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED, SDL_SCANCODE_SPACE,
                       "hit_space", &space_hit);
}

static const keybind_context beatup_contexts[] = {
     {.name = "game", .context = INPUT_CONTEXT_GAME},
     {.name = "console", .context = INPUT_CONTEXT_CONSOLE},
};

static const keybind_vocab beatup_vocab = {
     .contexts      = beatup_contexts,
     .context_count = sizeof(beatup_contexts) / sizeof(beatup_contexts[0]),
     .key_from_name = zngine_key_from_name,
     .key_to_name   = zngine_key_to_name,
};

// Console context has no game bindings, so typing there cannot pause the game.
static action_mode beatup_active_context(void) {
#if ZOD_CONSOLE_ENABLED
    if (zconsole_visible()) return INPUT_CONTEXT_CONSOLE;
#endif
    return INPUT_CONTEXT_GAME;
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
    beatup_actions_bind();
    zngine_keybind_manager_load(KEYBINDING_PATH, &beatup_vocab);
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
                     .identifier   = {.category = EVENT_CATEGORY_SYSTEM,
                                      .event_id = SYS_EVENT_ON_RESIZE_WINDOW},
                     .payload      = &payload,
                     .payload_size = sizeof(payload)};
                zngine_event_publish(&ctx);
            }
            if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
                const action_handle handle =
                     zngine_keybind_resolve(beatup_active_context(), (int)e.key.scancode,
                                            ACTION_TRIGGER_KEY_PRESSED);
                if (handle != ACTION_HANDLE_INVALID) zngine_action_execute(handle, NULL);
            }
            zngine_extensions_handle_event(&e);
        }
        zngine_input_update();
        zngine_tick_hot_reload();

        zngine_begin_drawing();
        beatup_layout_draw();
        beatup_hud_draw();

        render_text_flush();
        render_sprite_flush();
        zngine_extensions_draw();
        zngine_end_drawing();
        zngine_clock_sleep_to_target_fps();
    }
    beatup_layout_destroy();
    render_sprite_destroy();
    render_text_destroy();
    zngine_destroy();
    return 0;
}
