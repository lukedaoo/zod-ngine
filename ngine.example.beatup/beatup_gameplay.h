#ifndef BEATUP_GAMEPLAY_H
#define BEATUP_GAMEPLAY_H

#include "beatup_shared.h"

extern const keybind_vocab beatup_vocab;

void                   beatup_song_load(const char *path);
void                   beatup_music_load(const char *path);
void                   beatup_sfx_load(void);
void                   beatup_music_destroy(void);
void                   beatup_song_time_update(void);
void                   beatup_countdown_update(void);
void                   beatup_gameplay_update(float dt);
void                   beatup_autoplay_update(void);
void                   beatup_actions_bind(void);
action_mode            beatup_active_context(void);
bool                   load_args(const int argc, const char **argv, cvar_table *cvars);
command_execute_result beatup_cmd_autoplay(int argc, char **argv);

#endif
