#include "beatup_gameplay.h"

beatup_chart_note beatup_chart[BEATUP_MAX_CHART_NOTES];
int               beatup_chart_count      = 0;
int               beatup_first_avail_note = 0;
float             beatup_song_time_ms     = 0.0f;

beatup_lane beatup_lanes[6] = {
     {.right_col = false, .row = 0}, {.right_col = false, .row = 1},
     {.right_col = false, .row = 2}, {.right_col = true, .row = 0},
     {.right_col = true, .row = 1},  {.right_col = true, .row = 2},
};

float       beatup_score          = 0.0f;
int         beatup_combo          = 0;
int         beatup_max_combo      = 0;
static bool beatup_autoplay       = false;
int         beatup_judge_count[5] = {0};

float beatup_judge_timer;
int   beatup_judge_index;
bool  beatup_judge_active = false;

bool  beatup_space_explode_active     = false;
float beatup_space_explode_timer      = 0.0f;
bool  beatup_space_hit_explode_active = false;
float beatup_space_hit_explode_timer  = 0.0f;

bool  beatup_countdown_active   = false;
float beatup_first_note_time_ms = 0.0f;

void beatup_song_load(const char *path) {
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

    if (beatup_chart_count > 0) {
        beatup_first_note_time_ms = beatup_chart[0].time_ms;
        for (int i = 1; i < beatup_chart_count; i++)
            beatup_first_note_time_ms =
                 fminf(beatup_first_note_time_ms, beatup_chart[i].time_ms);
        beatup_countdown_active = true;
    }
}

static MIX_Mixer *beatup_mixer       = NULL;
static MIX_Audio *beatup_music_audio = NULL;
static MIX_Track *beatup_music_track = NULL;

void beatup_music_load(const char *path) {
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

void beatup_sfx_load(void) {
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

void beatup_music_destroy(void) {
    if (beatup_music_track) MIX_DestroyTrack(beatup_music_track);
    if (beatup_music_audio) MIX_DestroyAudio(beatup_music_audio);
    if (beatup_sfx_perfect) MIX_DestroyAudio(beatup_sfx_perfect);
    if (beatup_sfx_normal) MIX_DestroyAudio(beatup_sfx_normal);
    if (beatup_sfx_miss) MIX_DestroyAudio(beatup_sfx_miss);
    if (beatup_sfx_space) MIX_DestroyAudio(beatup_sfx_space);
    if (beatup_mixer) MIX_DestroyMixer(beatup_mixer);
    MIX_Quit();
}

void beatup_countdown_update(void) {
    if (!beatup_countdown_active) return;
    float remaining = beatup_first_note_time_ms - beatup_song_time_ms;
    if (remaining <= 4.0f * BEATUP_TICK_TIME_MS) beatup_countdown_active = false;
}

void beatup_song_time_update(void) {
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

static void beatup_apply_judgment(int key, int tier) {
    static const float lane_scores[5] = {BEATUP_SCORE_PERFECT, BEATUP_SCORE_GREAT,
                                         BEATUP_SCORE_COOL, BEATUP_SCORE_BAD, 0.0f};
    float points = key == 5 ? (tier != BEATUP_JUDGE_MISS ? BEATUP_SCORE_SPACE : 0.0f)
                            : lane_scores[tier];
    if (beatup_combo >= 400) {
        points *= BEATUP_SCORE_COMBO_MULT_400;
    } else if (beatup_combo >= 100) {
        points *= BEATUP_SCORE_COMBO_MULT_100;
    }
    beatup_score += points;
    beatup_judge_count[tier]++;
    if (tier == BEATUP_JUDGE_MISS) {
        beatup_combo = 0;
    } else {
        beatup_combo++;
    }
    if (beatup_combo > beatup_max_combo) beatup_max_combo = beatup_combo;
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

int beatup_lane_slot(int key) {
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

void beatup_gameplay_update(float dt) {
    (void)dt;
    int last = beatup_first_avail_note + BEATUP_LOOKAHEAD_NOTES;
    if (last > beatup_chart_count) last = beatup_chart_count;

    for (int i = beatup_first_avail_note; i < last; i++) {
        beatup_chart_note *note = &beatup_chart[i];
        if (note->pressed) continue;

        float time_to_hit = note->time_ms - beatup_song_time_ms;
        if (time_to_hit < -BEATUP_MISS_LATE_MS) {
            note->pressed = true;
            beatup_apply_judgment(note->key, BEATUP_JUDGE_MISS);
            if (beatup_sfx_miss) MIX_PlayAudio(beatup_mixer, beatup_sfx_miss);
            continue;
        }
        if (time_to_hit > BEATUP_ARROW_VISIBLE_MS) break;
    }
    beatup_advance_first_avail();
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
        } else if (key == 5 && tier != BEATUP_JUDGE_MISS) {
            beatup_space_hit_explode_active = true;
            beatup_space_hit_explode_timer  = 0.0f;
        }

        beatup_apply_judgment(key, tier);
        beatup_play_hit_sfx(key, tier);
        beatup_advance_first_avail();
        return;
    }
}

void beatup_autoplay_update(void) {
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

command_execute_result beatup_cmd_autoplay(int argc, char **argv) {
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

void beatup_actions_bind(void) {
    action_executor toggle_pause = {.execute = toggle_pause_execute};
    zngine_action_bind(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED, BEATUP_KEY_PAUSE,
                       "toggle_pause", &toggle_pause);

#if ZOD_CONSOLE_ENABLED
    action_executor toggle_console = {.execute = toggle_console_execute};
    zngine_action_bind(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED, BEATUP_KEY_CONSOLE,
                       "toggle_console", &toggle_console);
    zngine_action_bind(INPUT_CONTEXT_CONSOLE, ACTION_TRIGGER_KEY_PRESSED,
                       BEATUP_KEY_CONSOLE, "toggle_console", &toggle_console);
#endif

    action_executor lane_hit = {.execute = beatup_lane_hit_execute};
    static const struct {
        int         key;
        const char *name;
    } lane_binds[6] = {
         {BEATUP_KEY_HIT_LEFT_TOP, "hit_left_top"},
         {BEATUP_KEY_HIT_LEFT_MID, "hit_left_mid"},
         {BEATUP_KEY_HIT_LEFT_BOT, "hit_left_bot"},
         {BEATUP_KEY_HIT_RIGHT_TOP, "hit_right_top"},
         {BEATUP_KEY_HIT_RIGHT_MID, "hit_right_mid"},
         {BEATUP_KEY_HIT_RIGHT_BOT, "hit_right_bot"},
    };
    for (int i = 0; i < 6; i++) {
        zngine_action_bind(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED,
                           lane_binds[i].key, lane_binds[i].name, &lane_hit);
    }

    action_executor space_hit = {.execute = beatup_space_hit_execute};
    zngine_action_bind(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED,
                       BEATUP_KEY_HIT_SPACE, "hit_space", &space_hit);
}

static const keybind_context beatup_contexts[] = {
     {.name = "game", .context = INPUT_CONTEXT_GAME},
     {.name = "console", .context = INPUT_CONTEXT_CONSOLE},
};

const keybind_vocab beatup_vocab = {
     .contexts      = beatup_contexts,
     .context_count = sizeof(beatup_contexts) / sizeof(beatup_contexts[0]),
     .key_from_name = zngine_key_from_name,
     .key_to_name   = zngine_key_to_name,
};

action_mode beatup_active_context(void) {
#if ZOD_CONSOLE_ENABLED
    if (zconsole_visible()) return INPUT_CONTEXT_CONSOLE;
#endif
    return INPUT_CONTEXT_GAME;
}

bool load_args(const int argc, const char **argv, cvar_table *cvars) {
    carg_register defs[] = {
         {.flag = "--size", .arg_count = 2, .type = CARG_INT, .required = false},
    };

    carg_table cargs = {0};
    if (!carg_parse(defs, 1, argc, argv, &cargs)) return false;

    const char *size_names[] = {"window.width", "window.height"};
    return carg_entry_to_cvars(carg_get(&cargs, "--size"), size_names, 2, cvars);
}
