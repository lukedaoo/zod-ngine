#define NOB_IMPLEMENTATION
#include "lib/nob.h"

#define C_COMPILER "clang"
#define C_FLAGS    "-Wall", "-Wextra", "-std=c23"
#define C_TARGET   "./main"
#define C_ENTRY    "main.c"
#define SDL_FLAGS  "-I/usr/include/SDL3", "-lSDL3"

int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, C_COMPILER, C_FLAGS, SDL_FLAGS, "-o", C_TARGET,
                   C_ENTRY);
    if (!nob_cmd_run(&cmd)) return 1;

    if (argc > 1 && strcmp(argv[1], "run") == 0) {
        Nob_Cmd run = {0};
        nob_cmd_append(&run, C_TARGET);
        return !nob_cmd_run(&run);
    }

    return 0;
}
