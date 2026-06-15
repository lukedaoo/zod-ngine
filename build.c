#define NOB_IMPLEMENTATION
#include "lib/nob.h"

#define C_COMPILER      "cc"
#define C_FLAGS         "-Wall", "-Wextra", "-std=c23"
#define C_DEBUG_FLAGS   "-g", "-O0"
#define C_RELEASE_FLAGS "-O3", "-DNDEBUG"
#define C_TARGET        "./main"
#define C_ENTRY         "main.c"

#ifdef _WIN32
#define SDL_FLAGS "-I/ucrt64/include/SDL3", "-L/ucrt64/lib", "-lSDL3.dll"
#elif defined(__linux__)
#define SDL_FLAGS "-I/usr/include/SDL3", "-lSDL3"
#endif

int run_tests(void) {
#ifdef _WIN32
    if (!nob_mkdir_if_not_exists("tmp")) return 1;
#endif

    Nob_File_Paths modules = {0};
    if (!nob_read_entire_dir("modules", &modules)) return 1;

    for (size_t i = 0; i < modules.count; ++i) {
        const char *name = modules.items[i];
        if (!nob_sv_ends_with_cstr(nob_sv_from_cstr(name), "_test.c")) continue;

        const char *src = nob_temp_sprintf("modules/%s", name);
        const char *bin = nob_temp_sprintf("./%.*s.out", (int)(strlen(name) - 2), name);

        Nob_Cmd test_cmd = {0};
        nob_cmd_append(&test_cmd, C_COMPILER, C_FLAGS, "-o", bin, src);
        if (!nob_cmd_run(&test_cmd)) return 1;

        Nob_Cmd run_test = {0};
        nob_cmd_append(&run_test, bin);
        if (!nob_cmd_run(&run_test)) return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        return run_tests();
    }

    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, C_COMPILER, C_FLAGS, "-o", C_TARGET, C_ENTRY, SDL_FLAGS);
    if (argc > 1 && strcmp(argv[1], "debug") == 0) {
        nob_cmd_append(&cmd, C_DEBUG_FLAGS);
    } else if (argc > 1 && strcmp(argv[1], "release") == 0) {
        nob_cmd_append(&cmd, C_RELEASE_FLAGS);
    }
    if (!nob_cmd_run(&cmd)) return 1;

    if (argc > 1 && strcmp(argv[1], "run") == 0) {
        Nob_Cmd run = {0};
        nob_cmd_append(&run, C_TARGET);
        return !nob_cmd_run(&run);
    }

    return 0;
}
