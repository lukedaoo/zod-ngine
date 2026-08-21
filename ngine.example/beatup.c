#include <stdbool.h>

#include <SDL3_mixer/SDL_mixer.h>

#define ZOD_NGINE_IMPLEMENTATION
#include <ngine.core/index.h>
#include <ngine.ext.console/index.h>

#define CONFIG_PATH     "run-tree/data/engine_beatup.scf"
#define KEYBINDING_PATH "run-tree/data/keybind_beatup.scf"
#define SONG_PATH       "run-tree/data/song_1_beatup.scf"

#define BEATUP_ASSET_DIR "run-tree/data/textures/beatup/"
#define BEATUP_AUDIO_DIR "run-tree/data/audio/beatup/"
#define MUSIC_PATH       BEATUP_AUDIO_DIR "song1.ogg"

static float beatup_canvas_w = 1600.0f;
static float beatup_canvas_h = 900.0f;

#define BEATUP_SCALE 1.0f

#define BEATUP_TABLE_W    (123.0f * BEATUP_SCALE)
#define BEATUP_TABLE_TR   (3.0f * BEATUP_SCALE)
#define BEATUP_LANE_W     (256.0f * BEATUP_SCALE)
#define BEATUP_LANDING_W  (100.0f * BEATUP_SCALE)
#define BEATUP_CHANCE_DST (80.0f * BEATUP_SCALE)

#define BEATUP_COL_W     BEATUP_LANE_W
#define BEATUP_COL_H     (196.0f * BEATUP_SCALE)
#define BEATUP_NOTE_SIZE (64.0f * BEATUP_SCALE)
#define BEATUP_MARKER_W  (128.0f * BEATUP_SCALE)
#define BEATUP_MARKER_H  (64.0f * BEATUP_SCALE)
#define BEATUP_JUDGE_W   (256.0f * 1.25f * BEATUP_SCALE)
#define BEATUP_JUDGE_H   (128.0f * 1.25f * BEATUP_SCALE)

static sprite_texture beatup_lane_l, beatup_lane_r;
static sprite_texture beatup_landing_l, beatup_landing_r;
static sprite_texture beatup_marker;
static sprite_texture beatup_space_frame;
static sprite_texture beatup_space_explode;
static sprite_texture beatup_arrow_explode;
static bool           beatup_space_explode_active = false;
static float          beatup_space_explode_timer  = 0.0f;

static sprite_texture beatup_letter_tex[6];
static const char    *BEATUP_LETTER_PATHS[6] = {
     BEATUP_ASSET_DIR "space_frame_letter_b.png",
     BEATUP_ASSET_DIR "space_frame_letter_e.png",
     BEATUP_ASSET_DIR "space_frame_letter_a.png",
     BEATUP_ASSET_DIR "space_frame_letter_t.png",
     BEATUP_ASSET_DIR "space_frame_letter_u.png",
     BEATUP_ASSET_DIR "space_frame_letter_p.png",
};
static sprite_texture beatup_letter_glow_blue;
static sprite_texture beatup_letter_glow_yellow;
static sprite_texture beatup_frame_glow_blue;
static sprite_texture beatup_frame_glow_yellow;
#define BEATUP_LETTER_DIST (46.0f * BEATUP_SCALE)

static const char *BEATUP_JUDGE_PATHS[5] = {
     BEATUP_ASSET_DIR "perfect.png", BEATUP_ASSET_DIR "great.png",
     BEATUP_ASSET_DIR "cool.png",    BEATUP_ASSET_DIR "bad.png",
     BEATUP_ASSET_DIR "miss.png",
};
static sprite_texture beatup_judge_tex[5];
static float          beatup_judge_timer;
static int            beatup_judge_index;
static bool           beatup_judge_active = false;
#define BEATUP_JUDGE_HOLD 0.9f

enum : uint8_t {
    BEATUP_JUDGE_PERFECT,
    BEATUP_JUDGE_GREAT,
    BEATUP_JUDGE_COOL,
    BEATUP_JUDGE_BAD,
    BEATUP_JUDGE_MISS,
} beatup_judge_tier;

#define BEATUP_SCORE_PERFECT 100
#define BEATUP_SCORE_GREAT   75
#define BEATUP_SCORE_COOL    50
#define BEATUP_SCORE_BAD     25

static int  beatup_score    = 0;
static int  beatup_combo    = 0;
static bool beatup_autoplay = false;

static int beatup_judge_count[5] = {0};

#define BEATUP_BPM                          142.0f
#define BEATUP_TICK_TIME_MS                 (1000.0f * 60.0f / (BEATUP_BPM * 4.0f))
#define BEATUP_HIT_WINDOW_MS                (BEATUP_TICK_TIME_MS * 4.0f)
#define BEATUP_JUDGE_PERFECT_MS             (0.05f * BEATUP_HIT_WINDOW_MS)
#define BEATUP_JUDGE_GREAT_MS               (0.15f * BEATUP_HIT_WINDOW_MS)
#define BEATUP_JUDGE_COOL_MS                (0.27f * BEATUP_HIT_WINDOW_MS)
#define BEATUP_JUDGE_BAD_MS                 (0.40f * BEATUP_HIT_WINDOW_MS)
#define BEATUP_LATE_IGNORE_MS               (0.80f * BEATUP_HIT_WINDOW_MS)
#define BEATUP_MISS_LATE_MS                 (BEATUP_TICK_TIME_MS * 2.0f)
#define BEATUP_NOTE_SPEED_PX_PER_MS         (40.0f * BEATUP_SCALE / BEATUP_TICK_TIME_MS)
#define BEATUP_SPACE_CURSOR_SPEED_PX_PER_MS (15.5f * BEATUP_SCALE / BEATUP_TICK_TIME_MS)
#define BEATUP_LOOKAHEAD_NOTES              32
#define BEATUP_ARROW_VISIBLE_MS (BEATUP_TICK_TIME_MS * (BEATUP_LOOKAHEAD_NOTES + 1))
#define BEATUP_SPACE_VISIBLE_MS (BEATUP_TICK_TIME_MS * 8.0f)

typedef struct {
    uint8_t key;
    float   time_ms;
    bool    pressed;
} beatup_chart_note;

#define BEATUP_MAX_CHART_NOTES 4096
static beatup_chart_note beatup_chart[BEATUP_MAX_CHART_NOTES];
static int               beatup_chart_count      = 0;
static int               beatup_first_avail_note = 0;
static float             beatup_song_time_ms     = 0.0f;

static void beatup_song_load(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        log_error("beatup.song_load: failed to open '%s'", path);
        return;
    }

    char line[64];
    while (fgets(line, sizeof(line), f) && beatup_chart_count < BEATUP_MAX_CHART_NOTES) {
        if (line[0] == ':' || line[0] == '\n' || line[0] == '\0') continue;
        int key, tick;
        if (sscanf(line, "%d %d", &key, &tick) != 2) continue;
        beatup_chart[beatup_chart_count++] = (beatup_chart_note){
             .key     = (uint8_t)key,
             .time_ms = (float)tick * BEATUP_TICK_TIME_MS,
             .pressed = false  //
        };
    }
    fclose(f);
    log_info("beatup.song_load: loaded %d notes from '%s'", beatup_chart_count, path);
}

static MIX_Mixer *beatup_mixer       = NULL;
static MIX_Audio *beatup_music_audio = NULL;
static MIX_Track *beatup_music_track = NULL;

static void beatup_music_load(const char *path) {
    if (!MIX_Init()) {
        log_error("beatup.music_load: MIX_Init failed: %s", SDL_GetError());
        return;
    }

    beatup_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (!beatup_mixer) {
        log_error("beatup.music_load: MIX_CreateMixerDevice failed: %s", SDL_GetError());
        return;
    }

    beatup_music_audio = MIX_LoadAudio(beatup_mixer, path, false);
    if (!beatup_music_audio) {
        log_error("beatup.music_load: MIX_LoadAudio failed for '%s': %s", path,
                  SDL_GetError());
        return;
    }

    beatup_music_track = MIX_CreateTrack(beatup_mixer);
    if (!beatup_music_track ||
        !MIX_SetTrackAudio(beatup_music_track, beatup_music_audio)) {
        log_error("beatup.music_load: track setup failed: %s", SDL_GetError());
        return;
    }

    if (!MIX_PlayTrack(beatup_music_track, 0)) {
        log_error("beatup.music_load: MIX_PlayTrack failed: %s", SDL_GetError());
        return;
    }

    log_info("beatup.music_load: playing '%s'", path);
}

static MIX_Audio *beatup_sfx_perfect = NULL;
static MIX_Audio *beatup_sfx_normal  = NULL;
static MIX_Audio *beatup_sfx_miss    = NULL;
static MIX_Audio *beatup_sfx_space   = NULL;

static MIX_Audio *beatup_sfx_load_one(const char *path) {
    MIX_Audio *audio = MIX_LoadAudio(beatup_mixer, path, true);
    if (!audio) log_error("beatup.sfx_load: failed for '%s': %s", path, SDL_GetError());
    return audio;
}

static void beatup_sfx_load(void) {
    if (!beatup_mixer) return;
    beatup_sfx_perfect = beatup_sfx_load_one(BEATUP_AUDIO_DIR "perfect.wav");
    beatup_sfx_normal  = beatup_sfx_load_one(BEATUP_AUDIO_DIR "normal.wav");
    beatup_sfx_miss    = beatup_sfx_load_one(BEATUP_AUDIO_DIR "miss.wav");
    beatup_sfx_space   = beatup_sfx_load_one(BEATUP_AUDIO_DIR "space.wav");
}

static void beatup_play_hit_sfx(int key, int tier) {
    MIX_Audio *sfx = key == 5                       ? beatup_sfx_space
                     : tier == BEATUP_JUDGE_PERFECT ? beatup_sfx_perfect
                     : tier == BEATUP_JUDGE_MISS    ? beatup_sfx_miss
                                                    : beatup_sfx_normal;
    if (sfx) MIX_PlayAudio(beatup_mixer, sfx);
}

static void beatup_music_destroy(void) {
    if (beatup_music_track) MIX_DestroyTrack(beatup_music_track);
    if (beatup_music_audio) MIX_DestroyAudio(beatup_music_audio);
    if (beatup_sfx_perfect) MIX_DestroyAudio(beatup_sfx_perfect);
    if (beatup_sfx_normal) MIX_DestroyAudio(beatup_sfx_normal);
    if (beatup_sfx_miss) MIX_DestroyAudio(beatup_sfx_miss);
    if (beatup_sfx_space) MIX_DestroyAudio(beatup_sfx_space);
    if (beatup_mixer) MIX_DestroyMixer(beatup_mixer);
    MIX_Quit();
}

#define BEATUP_AUDIO_OFFSET_MS 0.0f

static void beatup_song_time_update(void) {
    if (beatup_music_track && MIX_TrackPlaying(beatup_music_track)) {
        Sint64 frames       = MIX_GetTrackPlaybackPosition(beatup_music_track);
        beatup_song_time_ms = (float)MIX_TrackFramesToMS(beatup_music_track, frames) +
                              BEATUP_AUDIO_OFFSET_MS;
    } else {
        beatup_song_time_ms += zngine_clock_dt() * 1000.0f;
    }
}

static void beatup_advance_first_avail(void) {
    while (beatup_first_avail_note < beatup_chart_count &&
           beatup_chart[beatup_first_avail_note].pressed)
        beatup_first_avail_note++;
}

static void beatup_apply_judgment(int tier) {
    static const int scores[5] = {BEATUP_SCORE_PERFECT, BEATUP_SCORE_GREAT,
                                  BEATUP_SCORE_COOL, BEATUP_SCORE_BAD, 0};
    beatup_score += scores[tier];
    beatup_judge_count[tier]++;
    if (tier == BEATUP_JUDGE_MISS) {
        beatup_combo = 0;
    } else {
        beatup_combo++;
    }
    beatup_judge_index  = tier;
    beatup_judge_timer  = 0.0f;
    beatup_judge_active = true;
}

static int beatup_classify_time_ms(float diff_ms) {
    if (beatup_autoplay) return BEATUP_JUDGE_PERFECT;
    if (diff_ms > BEATUP_LATE_IGNORE_MS || diff_ms < -BEATUP_HIT_WINDOW_MS) return -1;
    float abs_diff = fabsf(diff_ms);
    if (abs_diff <= BEATUP_JUDGE_PERFECT_MS) return BEATUP_JUDGE_PERFECT;
    if (abs_diff <= BEATUP_JUDGE_GREAT_MS) return BEATUP_JUDGE_GREAT;
    if (abs_diff <= BEATUP_JUDGE_COOL_MS) return BEATUP_JUDGE_COOL;
    if (abs_diff <= BEATUP_JUDGE_BAD_MS) return BEATUP_JUDGE_BAD;
    return BEATUP_JUDGE_MISS;
}

enum : uint8_t { INPUT_CONTEXT_GAME = 0, INPUT_CONTEXT_CONSOLE = 1 } input_contexts;

typedef struct {
    sprite_texture tex;
    const char    *path;
    sprite_texture hit_tex;
    const char    *hit_path;
    sprite_texture lane_down_tex;
    const char    *lane_down_path;
    bool           right_col;
    int            row;
    bool           flash_pending;
    float          flash_x;
    bool           overlay_active;
    float          overlay_timer;
    bool           lane_down_active;
    float          lane_down_timer;
} beatup_lane;

#define BEATUP_ARROW_ANIMATION_MS 0.07f

static beatup_lane beatup_lanes[6] = {
     {.path           = BEATUP_ASSET_DIR "a71.png",
      .hit_path       = BEATUP_ASSET_DIR "a75.png",
      .lane_down_path = BEATUP_ASSET_DIR "lane_7.png",
      .right_col      = false,
      .row            = 0},
     {.path           = BEATUP_ASSET_DIR "a41.png",
      .hit_path       = BEATUP_ASSET_DIR "a45.png",
      .lane_down_path = BEATUP_ASSET_DIR "lane_4.png",
      .right_col      = false,
      .row            = 1},
     {.path           = BEATUP_ASSET_DIR "a11.png",
      .hit_path       = BEATUP_ASSET_DIR "a15.png",
      .lane_down_path = BEATUP_ASSET_DIR "lane_1.png",
      .right_col      = false,
      .row            = 2},
     {.path           = BEATUP_ASSET_DIR "a91.png",
      .hit_path       = BEATUP_ASSET_DIR "a95.png",
      .lane_down_path = BEATUP_ASSET_DIR "lane_9.png",
      .right_col      = true,
      .row            = 0},
     {.path           = BEATUP_ASSET_DIR "a61.png",
      .hit_path       = BEATUP_ASSET_DIR "a65.png",
      .lane_down_path = BEATUP_ASSET_DIR "lane_6.png",
      .right_col      = true,
      .row            = 1},
     {.path           = BEATUP_ASSET_DIR "a31.png",
      .hit_path       = BEATUP_ASSET_DIR "a35.png",
      .lane_down_path = BEATUP_ASSET_DIR "lane_3.png",
      .right_col      = true,
      .row            = 2},
};

static int beatup_lane_slot(int key) {
    switch (key) {
        case 7:
            return 0;
        case 4:
            return 1;
        case 1:
            return 2;
        case 9:
            return 3;
        case 6:
            return 4;
        case 3:
            return 5;
        default:
            return -1;
    }
}

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

static float beatup_note_x_at(bool right_col, float time_to_hit_ms) {
    float landing_center = beatup_landing_x(right_col) + BEATUP_LANDING_W * 0.5f;
    float dir            = right_col ? 1.0f : -1.0f;
    return landing_center + dir * time_to_hit_ms * BEATUP_NOTE_SPEED_PX_PER_MS;
}

#define BEATUP_ARROW_LANE_OFS 1.0f

static float beatup_arrow_explode_center_x(bool right_col) {
    float near_edge =
         right_col ? beatup_landing_x(true) + BEATUP_LANDING_W : beatup_landing_x(false);
    float ofs = BEATUP_NOTE_SIZE * 0.5f + BEATUP_ARROW_LANE_OFS;
    return right_col ? near_edge - ofs : near_edge + ofs;
}

static float beatup_lane_down_x(bool right_col) {
    if (!right_col)
        return BEATUP_TABLE_W - BEATUP_TABLE_TR - BEATUP_CHANCE_DST -
               BEATUP_ARROW_LANE_OFS;
    return beatup_canvas_w - (BEATUP_TABLE_W - BEATUP_CHANCE_DST + BEATUP_LANE_W -
                              BEATUP_ARROW_LANE_OFS + BEATUP_NOTE_SIZE + 3.0f);
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

// B-E-A-T-U-P lit progressively by combo — ported from drawBeatupText_.
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

static void beatup_layout_init(void) {
    beatup_lane_l      = sprite_texture_load(BEATUP_ASSET_DIR "laneL.png");
    beatup_lane_r      = sprite_texture_load(BEATUP_ASSET_DIR "laneR.png");
    beatup_landing_l   = sprite_texture_load(BEATUP_ASSET_DIR "landingL.png");
    beatup_landing_r   = sprite_texture_load(BEATUP_ASSET_DIR "landingR.png");
    beatup_space_frame = sprite_texture_load(BEATUP_ASSET_DIR "space_frame.png");
    beatup_space_explode =
         sprite_texture_load(BEATUP_ASSET_DIR "space_frame_explode.png");
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
        beatup_lanes[i].tex     = sprite_texture_load(beatup_lanes[i].path);
        beatup_lanes[i].hit_tex = sprite_texture_load(beatup_lanes[i].hit_path);
        beatup_lanes[i].lane_down_tex =
             sprite_texture_load(beatup_lanes[i].lane_down_path);
    }
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

    float space_frame_w = 380.0f * BEATUP_SCALE;
    float space_frame_h = 128.0f * BEATUP_SCALE;
    float space_frame_x = (beatup_canvas_w - space_frame_w) / 2.0f;
    if (beatup_combo >= 400) {
        render_sprite_draw(beatup_frame_glow_blue, space_frame_x, space_frame_y,
                           space_frame_w, space_frame_h, COLOR4F_WHITE);
    } else if (beatup_combo >= 100) {
        render_sprite_draw(beatup_frame_glow_yellow, space_frame_x, space_frame_y,
                           space_frame_w, space_frame_h, COLOR4F_WHITE);
    }
    render_sprite_draw(beatup_space_frame, space_frame_x, space_frame_y, space_frame_w,
                       space_frame_h, COLOR4F_WHITE);
    beatup_letters_draw(beatup_canvas_w * 0.5f, space_frame_y + space_frame_h * 0.5f);
    float cursor_y            = space_frame_y + (space_frame_h - BEATUP_MARKER_H) / 2.0f;
    float space_cursor_base_x = (beatup_canvas_w - BEATUP_MARKER_W) / 2.0f;

    static const float row_yofs[3] = {3.0f, 67.0f, 131.0f};
    float              dt          = zngine_clock_dt();

    int last = beatup_first_avail_note + BEATUP_LOOKAHEAD_NOTES;
    if (last > beatup_chart_count) last = beatup_chart_count;

    for (int i = beatup_first_avail_note; i < last; i++) {
        beatup_chart_note *note = &beatup_chart[i];
        if (note->pressed) continue;

        float time_to_hit = note->time_ms - beatup_song_time_ms;

        if (time_to_hit < -BEATUP_MISS_LATE_MS) {
            note->pressed = true;
            beatup_apply_judgment(BEATUP_JUDGE_MISS);
            if (beatup_sfx_miss) MIX_PlayAudio(beatup_mixer, beatup_sfx_miss);
            continue;
        }

        if (time_to_hit > BEATUP_ARROW_VISIBLE_MS) break;

        if (note->key == 5) {
            if (time_to_hit > BEATUP_SPACE_VISIBLE_MS) continue;
            float ofs = time_to_hit * BEATUP_SPACE_CURSOR_SPEED_PX_PER_MS;
            render_sprite_draw(beatup_marker, space_cursor_base_x - ofs, cursor_y,
                               BEATUP_MARKER_W, BEATUP_MARKER_H, COLOR4F_WHITE);
            render_sprite_draw(beatup_marker, space_cursor_base_x + ofs, cursor_y,
                               BEATUP_MARKER_W, BEATUP_MARKER_H, COLOR4F_WHITE);
            continue;
        }

        int slot = beatup_lane_slot(note->key);
        if (slot < 0) continue;
        beatup_lane *lane  = &beatup_lanes[slot];
        float        row_y = col_top + row_yofs[lane->row] * BEATUP_SCALE;
        float        x =
             beatup_note_x_at(lane->right_col, time_to_hit) - BEATUP_NOTE_SIZE * 0.5f;
        render_sprite_draw(lane->tex, x, row_y, BEATUP_NOTE_SIZE, BEATUP_NOTE_SIZE,
                           COLOR4F_WHITE);
    }
    beatup_advance_first_avail();

    for (int i = 0; i < 6; i++) {
        if (beatup_lanes[i].flash_pending) {
            beatup_lanes[i].flash_pending = false;
            float row_y = col_top + row_yofs[beatup_lanes[i].row] * BEATUP_SCALE;
            render_sprite_draw(beatup_lanes[i].hit_tex,
                               beatup_lanes[i].flash_x - BEATUP_NOTE_SIZE * 0.5f, row_y,
                               BEATUP_NOTE_SIZE, BEATUP_NOTE_SIZE, COLOR4F_WHITE);
        }

        bool  right        = beatup_lanes[i].right_col;
        float row_y        = col_top + row_yofs[beatup_lanes[i].row] * BEATUP_SCALE;
        float row_center_y = row_y + BEATUP_NOTE_SIZE * 0.5f;

        if (beatup_lanes[i].lane_down_active) {
            beatup_lanes[i].lane_down_timer += dt;
            if (beatup_lanes[i].lane_down_timer >= BEATUP_ARROW_ANIMATION_MS) {
                beatup_lanes[i].lane_down_active = false;
            } else {
                float lane_down_w =
                     (float)beatup_lanes[i].lane_down_tex.width * BEATUP_SCALE;
                float lane_down_h =
                     (float)beatup_lanes[i].lane_down_tex.height * BEATUP_SCALE;
                render_sprite_draw(beatup_lanes[i].lane_down_tex,
                                   beatup_lane_down_x(right),
                                   row_center_y - lane_down_h * 0.5f, lane_down_w,
                                   lane_down_h, COLOR4F_WHITE);
            }
        }

        if (beatup_lanes[i].overlay_active) {
            beatup_lanes[i].overlay_timer += dt;
            if (beatup_lanes[i].overlay_timer >= BEATUP_ARROW_ANIMATION_MS) {
                beatup_lanes[i].overlay_active = false;
            } else {
                float explode_w = (float)beatup_arrow_explode.width * BEATUP_SCALE;
                float explode_h = (float)beatup_arrow_explode.height * BEATUP_SCALE;
                render_sprite_draw(
                     beatup_arrow_explode,
                     beatup_arrow_explode_center_x(right) - explode_w * 0.5f,
                     row_center_y - explode_h * 0.5f, explode_w, explode_h,
                     COLOR4F_WHITE);
            }
        }
    }

    if (beatup_judge_active) {
        beatup_judge_timer += dt;
        if (beatup_judge_timer >= BEATUP_JUDGE_HOLD) beatup_judge_active = false;

        // Pop-in scale, ported from drawBigNoteResultText_ — grows from
        // ~1.56x down to 1x over the first 50ms, then holds flat.
        float ratio = 1.0f;
        if (beatup_judge_timer < 0.05f)
            ratio = 1.0f + (0.05f - beatup_judge_timer) / 0.09f;

        float judge_w        = BEATUP_JUDGE_W * ratio;
        float judge_h        = BEATUP_JUDGE_H * ratio;
        float judge_center_x = beatup_canvas_w * 0.5f;
        float judge_center_y = judge_y + BEATUP_JUDGE_H * 0.5f;
        render_sprite_draw(
             beatup_judge_tex[beatup_judge_index], judge_center_x - judge_w * 0.5f,
             judge_center_y - judge_h * 0.5f, judge_w, judge_h, COLOR4F_WHITE);
    }

    if (beatup_space_explode_active) {
        beatup_space_explode_timer += dt;
        if (beatup_space_explode_timer >= BEATUP_ARROW_ANIMATION_MS) {
            beatup_space_explode_active = false;
        } else {
            render_sprite_draw(beatup_space_explode, space_frame_x, space_frame_y,
                               space_frame_w, space_frame_h, COLOR4F_WHITE);
        }
    }
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

static void beatup_hud_draw(void) {
    float content_h   = 492.0f;
    float content_top = (beatup_canvas_h - content_h) * 0.5f;
    float hud_score_y = content_top + 180.0f;
    float hud_combo_y = content_top + 224.0f;
    float hud_stats_y = content_top + 268.0f;

    const simple_font *font      = zngine_font_primary_get();
    float              font_size = 32.0f;
    float              scale     = font_size / (float)simple_font_get_advance(font);

    char score_buf[32];
    char combo_buf[32];
    char stats_buf[80];
    snprintf(score_buf, sizeof(score_buf), "Score: %d", beatup_score);
    snprintf(combo_buf, sizeof(combo_buf), "Combo: %d", beatup_combo);
    snprintf(stats_buf, sizeof(stats_buf), "P/G/C/B/M: %d/%d/%d/%d/%d",
             beatup_judge_count[BEATUP_JUDGE_PERFECT],
             beatup_judge_count[BEATUP_JUDGE_GREAT],
             beatup_judge_count[BEATUP_JUDGE_COOL], beatup_judge_count[BEATUP_JUDGE_BAD],
             beatup_judge_count[BEATUP_JUDGE_MISS]);

    float widest = beatup_text_width(font, score_buf, scale);
    widest       = fmaxf(widest, beatup_text_width(font, combo_buf, scale));
    widest       = fmaxf(widest, beatup_text_width(font, stats_buf, scale));
    float hud_x  = beatup_canvas_w * 0.5f - widest * 0.5f;

    render_text_draw_basic(hud_x, hud_score_y, score_buf, scale, COLOR4F_WHITE, font,
                           0.0f);
    render_text_draw_basic(hud_x, hud_combo_y, combo_buf, scale, COLOR4F_WHITE, font,
                           0.0f);
    render_text_draw_basic(hud_x, hud_stats_y, stats_buf, scale, COLOR4F_WHITE, font,
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
    if (beatup_music_track) {
        if (g_ctx.clock.paused) {
            MIX_PauseTrack(beatup_music_track);
        } else {
            MIX_ResumeTrack(beatup_music_track);
        }
    }
    return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
}

static void beatup_try_hit(int key) {
    int end = beatup_first_avail_note + BEATUP_LOOKAHEAD_NOTES;
    if (end > beatup_chart_count) end = beatup_chart_count;

    for (int i = beatup_first_avail_note; i < end; i++) {
        beatup_chart_note *note = &beatup_chart[i];
        if (note->pressed || note->key != key) continue;

        float diff_ms = beatup_song_time_ms - note->time_ms;
        int   tier    = beatup_classify_time_ms(diff_ms);
        if (tier < 0) return;

        note->pressed = true;

        if (key != 5 && tier != BEATUP_JUDGE_MISS) {
            int slot = beatup_lane_slot(key);
            if (slot >= 0) {
                beatup_lanes[slot].flash_pending  = true;
                beatup_lanes[slot].overlay_active = true;
                beatup_lanes[slot].overlay_timer  = 0.0f;
                beatup_lanes[slot].flash_x        = beatup_note_x_at(
                     beatup_lanes[slot].right_col, note->time_ms - beatup_song_time_ms);
            }
        }

        beatup_apply_judgment(tier);
        beatup_play_hit_sfx(key, tier);
        beatup_advance_first_avail();
        return;
    }
}

static void beatup_autoplay_update(void) {
    if (!beatup_autoplay) return;
    if (beatup_first_avail_note >= beatup_chart_count) return;

    beatup_chart_note *note = &beatup_chart[beatup_first_avail_note];
    if (note->time_ms > beatup_song_time_ms + 5.0f) return;

    int key = note->key;
    if (key == 5) {
        beatup_space_explode_active = true;
        beatup_space_explode_timer  = 0.0f;
    } else {
        int slot = beatup_lane_slot(key);
        if (slot >= 0) {
            beatup_lanes[slot].lane_down_active = true;
            beatup_lanes[slot].lane_down_timer  = 0.0f;
        }
    }
    beatup_try_hit(key);
}

static int beatup_action_name_to_key(const char *name) {
    if (strcmp(name, "hit_left_top") == 0) return 7;
    if (strcmp(name, "hit_left_mid") == 0) return 4;
    if (strcmp(name, "hit_left_bot") == 0) return 1;
    if (strcmp(name, "hit_right_top") == 0) return 9;
    if (strcmp(name, "hit_right_mid") == 0) return 6;
    if (strcmp(name, "hit_right_bot") == 0) return 3;
    return 0;
}

static action_execute_result beatup_lane_hit_execute(const action_table *table,
                                                     action_handle self, void *userdata) {
    (void)userdata;
    if (beatup_autoplay)
        return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
    action_info info;
    if (action_lookup(table, self, &info)) {
        int key = beatup_action_name_to_key(info.name);
        if (key != 0) {
            int slot = beatup_lane_slot(key);
            if (slot >= 0) {
                beatup_lanes[slot].lane_down_active = true;
                beatup_lanes[slot].lane_down_timer  = 0.0f;
            }
            beatup_try_hit(key);
        }
    }
    return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
}

static action_execute_result beatup_space_hit_execute(const action_table *table,
                                                      action_handle       self,
                                                      void               *userdata) {
    (void)table;
    (void)self;
    (void)userdata;
    if (beatup_autoplay)
        return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
    beatup_space_explode_active = true;
    beatup_space_explode_timer  = 0.0f;
    beatup_try_hit(5);
    return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
}

static command_execute_result beatup_cmd_autoplay(int argc, char **argv) {
    if (argc >= 1) {
        if (strcmp(argv[0], "on") == 0) {
            beatup_autoplay = true;
        } else if (strcmp(argv[0], "off") == 0) {
            beatup_autoplay = false;
        } else {
            return (command_execute_result){.type      = COMMAND_RESULT_ERROR,
                                            .value.str = "usage: autoplay [on|off]"};
        }
    } else {
        beatup_autoplay = !beatup_autoplay;
    }
    return (command_execute_result){
         .type      = COMMAND_RESULT_STRING,
         .value.str = beatup_autoplay ? "autoplay: on" : "autoplay: off"};
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
    zngine_command_register(COMMAND_GROUP_USER_DEFINED, "autoplay", beatup_cmd_autoplay);
    zngine_keybind_manager_load(KEYBINDING_PATH, &beatup_vocab);
    render_text_init();
    render_sprite_init();
    beatup_layout_init();
    beatup_song_load(SONG_PATH);
    beatup_music_load(MUSIC_PATH);
    beatup_sfx_load();

    while (!zngine_should_exit()) {
        zngine_clock_update();
        beatup_song_time_update();
        beatup_autoplay_update();
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
    beatup_music_destroy();
    beatup_layout_destroy();
    render_sprite_destroy();
    render_text_destroy();
    zngine_destroy();
    return 0;
}
