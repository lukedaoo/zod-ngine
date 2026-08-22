#ifndef BEATUP_CONFIG_H
#define BEATUP_CONFIG_H

#include <stdbool.h>

#include <SDL3_mixer/SDL_mixer.h>

#define ZOD_NGINE_IMPLEMENTATION
#include <ngine.core/index.h>
#include <ngine.ext.console/index.h>

#define BEATUP_CONFIG_DIR "run-tree/data/beatup/config/"
#define CONFIG_PATH       BEATUP_CONFIG_DIR "engine_beatup.scf"
#define KEYBINDING_PATH   BEATUP_CONFIG_DIR "keybind_beatup.scf"
#define SONG_PATH         BEATUP_CONFIG_DIR "song_1_beatup.scf"

#define BEATUP_ASSET_DIR "run-tree/data/beatup/texture/"
#define BEATUP_AUDIO_DIR "run-tree/data/beatup/audio/"
#define MUSIC_PATH       BEATUP_AUDIO_DIR "song1.ogg"

// --- Default keybinds --------------------------------------------------
#ifndef BEATUP_KEY_PAUSE
#define BEATUP_KEY_PAUSE SDL_SCANCODE_P
#endif
#ifndef BEATUP_KEY_CONSOLE
#define BEATUP_KEY_CONSOLE SDL_SCANCODE_GRAVE
#endif
#ifndef BEATUP_KEY_HIT_LEFT_TOP
#define BEATUP_KEY_HIT_LEFT_TOP SDL_SCANCODE_R
#endif
#ifndef BEATUP_KEY_HIT_LEFT_MID
#define BEATUP_KEY_HIT_LEFT_MID SDL_SCANCODE_F
#endif
#ifndef BEATUP_KEY_HIT_LEFT_BOT
#define BEATUP_KEY_HIT_LEFT_BOT SDL_SCANCODE_V
#endif
#ifndef BEATUP_KEY_HIT_RIGHT_TOP
#define BEATUP_KEY_HIT_RIGHT_TOP SDL_SCANCODE_I
#endif
#ifndef BEATUP_KEY_HIT_RIGHT_MID
#define BEATUP_KEY_HIT_RIGHT_MID SDL_SCANCODE_K
#endif
#ifndef BEATUP_KEY_HIT_RIGHT_BOT
#define BEATUP_KEY_HIT_RIGHT_BOT SDL_SCANCODE_M
#endif
#ifndef BEATUP_KEY_HIT_SPACE
#define BEATUP_KEY_HIT_SPACE SDL_SCANCODE_SPACE
#endif

// --- Layout scale ------------------------------------------------------
#ifndef BEATUP_SCALE
#define BEATUP_SCALE 1.0f
#endif

#define BEATUP_TABLE_W    (123.0f * BEATUP_SCALE)
#define BEATUP_TABLE_TR   (8.0f * BEATUP_SCALE)
#define BEATUP_LANE_W     (256.0f * BEATUP_SCALE)
#define BEATUP_LANDING_W  (100.0f * BEATUP_SCALE)
#define BEATUP_CHANCE_DST (80.0f * BEATUP_SCALE)

#define BEATUP_COL_H     (196.0f * BEATUP_SCALE)
#define BEATUP_NOTE_SIZE (64.0f * BEATUP_SCALE)
#define BEATUP_MARKER_W  (128.0f * BEATUP_SCALE)
#define BEATUP_MARKER_H  (64.0f * BEATUP_SCALE)
#define BEATUP_JUDGE_W   (256.0f * 1.25f * BEATUP_SCALE)
#define BEATUP_JUDGE_H   (128.0f * 1.25f * BEATUP_SCALE)

#define BEATUP_LETTER_DIST        (46.0f * BEATUP_SCALE)
#define BEATUP_ARROW_LANE_OFS     1.0f
#define BEATUP_ARROW_ANIMATION_MS 0.07f
#define BEATUP_JUDGE_HOLD         0.9f

// --- Judgment / timing ---------------------------------------------------

#ifndef BEATUP_BPM
#define BEATUP_BPM 142.0f
#endif
#define BEATUP_TICK_TIME_MS                 (1000.0f * 60.0f / (BEATUP_BPM * 4.0f))
#define BEATUP_HIT_WINDOW_MS                (BEATUP_TICK_TIME_MS * 4.0f)
#define BEATUP_JUDGE_PERFECT_MS             (0.05f * BEATUP_HIT_WINDOW_MS)
#define BEATUP_JUDGE_GREAT_MS               (0.15f * BEATUP_HIT_WINDOW_MS)
#define BEATUP_JUDGE_COOL_MS                (0.27f * BEATUP_HIT_WINDOW_MS)
#define BEATUP_JUDGE_BAD_MS                 (0.40f * BEATUP_HIT_WINDOW_MS)
#define BEATUP_LATE_IGNORE_MS               (0.80f * BEATUP_HIT_WINDOW_MS)
#define BEATUP_MISS_LATE_MS                 (BEATUP_TICK_TIME_MS * 2.0f)
#define BEATUP_NOTE_SPEED_PX_PER_MS         (40.0f * BEATUP_SCALE / BEATUP_TICK_TIME_MS)
#define BEATUP_NOTE_TRAVEL_MS (BEATUP_LANE_W / BEATUP_NOTE_SPEED_PX_PER_MS)
#define BEATUP_SPACE_CURSOR_SPEED_PX_PER_MS (15.5f * BEATUP_SCALE / BEATUP_TICK_TIME_MS)
#define BEATUP_LOOKAHEAD_NOTES              32
#define BEATUP_ARROW_VISIBLE_MS (BEATUP_TICK_TIME_MS * (BEATUP_LOOKAHEAD_NOTES + 1))
#define BEATUP_SPACE_VISIBLE_MS (BEATUP_TICK_TIME_MS * 8.0f)
#define BEATUP_AUDIO_OFFSET_MS  0.0f

#define BEATUP_SCORE_PERFECT        480.0f
#define BEATUP_SCORE_GREAT          240.0f
#define BEATUP_SCORE_COOL           120.0f
#define BEATUP_SCORE_BAD            60.0f
#define BEATUP_SCORE_SPACE          2000.0f
#define BEATUP_SCORE_COMBO_MULT_100 1.2f
#define BEATUP_SCORE_COMBO_MULT_400 1.44f

#define BEATUP_MAX_CHART_NOTES 4096
#endif
