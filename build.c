#define NOB_IMPLEMENTATION
#include "lib/nob.h"

#define C_COMPILER      "cc"
#define C_FLAGS         "-Wall", "-Wextra", "-std=c23"
#define C_DEBUG_FLAGS   "-g", "-O0", "-DDEBUG"
#define C_ASAN_FLAGS    "-g", "-O0", "-fsanitize=address"
#define C_RELEASE_FLAGS "-O3", "-DNDEBUG"

#ifdef _WIN32

#define EXE_EXT       ".exe"
#define C_TARGET      "main"
#define ENGINE_TARGET "engine_run"

#else

#define EXE_EXT       ""
#define C_TARGET      "./main"
#define ENGINE_TARGET "./engine_run"

#endif

#define C_ENTRY      "main.c"
#define ENGINE_ENTRY "ngine/zod_ngine_run.c"

#ifdef _WIN32
#define SDL_FLAGS "-I/ucrt64/include/SDL3", "-L/ucrt64/lib", "-lSDL3.dll"
#elif defined(__linux__)
#define SDL_FLAGS "-I/usr/include/SDL3", "-lSDL3"
#endif

int build_test_dir(const char *dir) {
    Nob_File_Paths files = {0};
    if (!nob_read_entire_dir(dir, &files)) return 1;

    for (size_t i = 0; i < files.count; ++i) {
        const char *name = files.items[i];
        if (!nob_sv_starts_with(nob_sv_from_cstr(name), nob_sv_from_cstr("test_")))
            continue;

        const char *src = nob_temp_sprintf("%s/%s", dir, name);
        const char *bin =
             nob_temp_sprintf("./%.*s.out%s", (int)(strlen(name) - 2), name, EXE_EXT);

        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, C_COMPILER, C_FLAGS, C_DEBUG_FLAGS, "-o", bin, src);
        if (!nob_cmd_run(&cmd)) return 1;
    }
    return 0;
}

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

static const char  *TEST_DIRS[]     = {"modules", "modules/collections", "ngine"};
static const size_t TEST_DIRS_COUNT = sizeof(TEST_DIRS) / sizeof(TEST_DIRS[0]);

int run_tests(bool asan, const char *dir) {
#ifdef _WIN32
    if (!nob_mkdir_if_not_exists("tmp")) return 1;
#endif
    if (dir) {
        bool found = false;
        for (size_t i = 0; i < TEST_DIRS_COUNT; ++i) {
            if (strcmp(dir, TEST_DIRS[i]) == 0 ||
                nob_sv_starts_with(nob_sv_from_cstr(TEST_DIRS[i]),
                                   nob_sv_from_cstr(dir))) {
                run_test_dir(asan, TEST_DIRS[i]);
                found = true;
            }
        }
        if (!found) {
            nob_log(NOB_ERROR,
                    "unknown test dir '%s'. use: ./nob test modules | ./nob test ngine",
                    dir);
            return 1;
        }
        return 0;
    }
    for (size_t i = 0; i < TEST_DIRS_COUNT; ++i) run_test_dir(asan, TEST_DIRS[i]);
    return 0;
}

int run_clean(void) {
    for (size_t i = 0; i < TEST_DIRS_COUNT; ++i) {
        Nob_File_Paths files = {0};
        if (!nob_read_entire_dir(TEST_DIRS[i], &files)) continue;
        for (size_t j = 0; j < files.count; ++j) {
            const char *name = files.items[j];
            if (!nob_sv_starts_with(nob_sv_from_cstr(name), nob_sv_from_cstr("test_")))
                continue;
            const char *bin =
                 nob_temp_sprintf("./%.*s.out%s", (int)(strlen(name) - 2), name, EXE_EXT);
            if (nob_file_exists(bin) > 0) nob_delete_file(bin);
        }
    }
    if (nob_file_exists(C_TARGET) > 0) nob_delete_file(C_TARGET);
    if (nob_file_exists(ENGINE_TARGET) > 0) nob_delete_file(ENGINE_TARGET);
    return 0;
}

int run_run(bool execute, const char *target, const char *mode) {
    if (strcmp(mode, "debug") != 0 && strcmp(mode, "release") != 0) {
        nob_log(NOB_ERROR, "unknown build mode '%s'. use: debug | release", mode);
        return 1;
    }
    bool        is_engine = strcmp(target, "engine") == 0;
    const char *out       = is_engine ? ENGINE_TARGET : C_TARGET;
    const char *src       = is_engine ? ENGINE_ENTRY : C_ENTRY;

    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, C_COMPILER, C_FLAGS, "-o", out, src, SDL_FLAGS);
    if (strcmp(mode, "release") == 0) {
        nob_cmd_append(&cmd, C_RELEASE_FLAGS);
    } else {
        nob_cmd_append(&cmd, C_DEBUG_FLAGS);
    }
    if (!nob_cmd_run(&cmd)) return 1;
    if (execute) {
        Nob_Cmd run = {0};
        nob_cmd_append(&run, out);
        return !nob_cmd_run(&run);
    }
    return 0;
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (argc > 1 && strcmp(argv[1], "help") == 0) {
        nob_log(NOB_INFO, "usage: ./nob [command] [options]");
        nob_log(NOB_INFO, "  (none)                        build main debug");
        nob_log(NOB_INFO, "  run [engine] [debug|release]  build and run");
        nob_log(NOB_INFO, "  engine [debug|release]        build engine_run");
        nob_log(NOB_INFO, "  debug|release                 build main with mode");
        nob_log(NOB_INFO, "  test [dir]                    run tests (modules, ngine)");
        nob_log(NOB_INFO, "  test-asan [dir]               run tests with asan");
        nob_log(NOB_INFO, "  clean                         remove build artifacts");
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "test-build") == 0) {
        const char *dir = argc > 2 ? argv[2] : NULL;
        if (dir) return build_test_dir(dir);
        for (size_t i = 0; i < TEST_DIRS_COUNT; ++i) build_test_dir(TEST_DIRS[i]);
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        return run_tests(false, argc > 2 ? argv[2] : NULL);
    }

    if (argc > 1 && strcmp(argv[1], "test-asan") == 0) {
        return run_tests(true, argc > 2 ? argv[2] : NULL);
    }

    if (argc > 1 && strcmp(argv[1], "clean") == 0) {
        return run_clean();
    }

    bool do_run = argc > 1 && strcmp(argv[1], "run") == 0;

    if (do_run) {
        bool engine = argc > 2 && strcmp(argv[2], "engine") == 0;
        // clang-format off
        const char *mode = engine
            ? (argc > 3 ? argv[3] : "debug")
            : (argc > 2 ? argv[2] : "debug");
        // clang-format on
        return run_run(true, engine ? "engine" : "main", mode);
    }

    if (argc > 1 && strcmp(argv[1], "engine") == 0) {
        const char *mode = argc > 2 ? argv[2] : "debug";
        return run_run(false, "engine", mode);
    }

    // clang-format off
    const char *mode = argc > 1 ? argv[1] : "debug";
    // clang-format on
    return run_run(false, "main", mode);
}
