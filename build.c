#define NOB_IMPLEMENTATION
#include "lib/nob.h"

#define C_COMPILER      "cc"
#define C_FLAGS         "-Wall", "-Wextra", "-std=c23"
#define C_DEBUG_FLAGS   "-g", "-O0", "-DDEBUG"
#define C_ASAN_FLAGS    "-g", "-O0", "-fsanitize=address"
#define C_RELEASE_FLAGS "-O3", "-DNDEBUG"

#ifdef _WIN32
#define EXE_EXT  ".exe"
#define C_TARGET "main"
#else
#define EXE_EXT  ""
#define C_TARGET "./main"
#endif

#define C_ENTRY "main.c"

#ifdef _WIN32
#define SDL_FLAGS "-I/ucrt64/include/SDL3", "-L/ucrt64/lib", "-lSDL3.dll"
#elif defined(__linux__)
#define SDL_FLAGS "-I/usr/include/SDL3", "-lSDL3"
#endif

int run_test_dir(bool asan, const char *dir) {
    Nob_File_Paths files = {0};
    if (!nob_read_entire_dir(dir, &files)) return 1;

    for (size_t i = 0; i < files.count; ++i) {
        const char *name = files.items[i];
        if (!nob_sv_starts_with(nob_sv_from_cstr(name), nob_sv_from_cstr("test_")))
            continue;

        const char *src = nob_temp_sprintf("%s/%s", dir, name);
        const char *bin =
             nob_temp_sprintf("./%.*s.out%s", (int)(strlen(name) - 2), name, EXE_EXT);

        Nob_Cmd test_cmd = {0};
        nob_cmd_append(&test_cmd, C_COMPILER, C_FLAGS, "-o", bin, src);
        if (asan) nob_cmd_append(&test_cmd, C_ASAN_FLAGS);
        if (!nob_cmd_run(&test_cmd)) return 1;

        Nob_Cmd run_test = {0};
        nob_cmd_append(&run_test, bin);
        if (!nob_cmd_run(&run_test)) return 1;
    }
    return 0;
}

int run_tests(bool asan) {
#ifdef _WIN32
    if (!nob_mkdir_if_not_exists("tmp")) return 1;
#endif
    run_test_dir(asan, "modules");
    run_test_dir(asan, "modules/collections");
    return 0;
}

int run_clean(void) {
    Nob_File_Paths modules = {0};
    if (!nob_read_entire_dir("modules", &modules)) return 1;

    for (size_t i = 0; i < modules.count; ++i) {
        const char *name = modules.items[i];
        if (nob_sv_starts_with(nob_sv_from_cstr(name), nob_sv_from_cstr("test_"))) {
            const char *bin =
                 nob_temp_sprintf("./%.*s.out%s", (int)(strlen(name) - 2), name, EXE_EXT);
            if (nob_file_exists(bin) > 0) nob_delete_file(bin);
        }
    }
    if (nob_file_exists(C_TARGET) > 0) nob_delete_file(C_TARGET);
    return 0;
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        return run_tests(false);
    }

    if (argc > 1 && strcmp(argv[1], "asan") == 0) {
        return run_tests(true);
    }

    if (argc > 1 && strcmp(argv[1], "clean") == 0) {
        return run_clean();
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
