#ifndef BEATUP_SHARED_H
#define BEATUP_SHARED_H

#include "config.h"

enum : uint8_t { INPUT_CONTEXT_GAME = 0, INPUT_CONTEXT_CONSOLE = 1 } input_contexts;

enum : uint8_t {
    BEATUP_JUDGE_PERFECT,
    BEATUP_JUDGE_GREAT,
    BEATUP_JUDGE_COOL,
    BEATUP_JUDGE_BAD,
    BEATUP_JUDGE_MISS,
} beatup_judge_tier;

typedef struct {
    uint8_t key;  // 1,3,4,6,7,9 = lane, 5 = space
    float   time_ms;
    bool    pressed;
} beatup_chart_note;

typedef struct {
    sprite_texture tex;
    sprite_texture hit_tex;
    sprite_texture lane_down_tex;
    bool           right_col;
    int            row;
    bool           flash_pending;
    float          flash_x;
    bool           overlay_active;
    float          overlay_timer;
    bool           lane_down_active;
    float          lane_down_timer;
} beatup_lane;

extern beatup_chart_note beatup_chart[BEATUP_MAX_CHART_NOTES];
extern int               beatup_chart_count;
extern int               beatup_first_avail_note;
extern float             beatup_song_time_ms;
extern beatup_lane       beatup_lanes[6];
extern float             beatup_score;
extern int               beatup_combo;
extern int               beatup_max_combo;
extern int               beatup_judge_count[5];
extern float             beatup_judge_timer;
extern int               beatup_judge_index;
extern bool              beatup_judge_active;
extern bool              beatup_space_explode_active;
extern float             beatup_space_explode_timer;
extern bool              beatup_space_hit_explode_active;
extern float             beatup_space_hit_explode_timer;
extern bool              beatup_countdown_active;
extern float             beatup_first_note_time_ms;

int   beatup_lane_slot(int key);
float beatup_note_x_at(bool right_col, float time_to_hit_ms);

#endif
