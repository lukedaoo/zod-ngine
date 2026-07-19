#define NOB_IMPLEMENTATION
#include "thirdparty/nob.h"

#define C_COMPILER      "cc"
#define C_FLAGS         "-Wall", "-Wextra", "-std=c23", "-I."
#define C_DEBUG_FLAGS   "-g", "-O0", "-DDEBUG"
#define C_ASAN_FLAGS    "-g", "-O0", "-fsanitize=address"
#define C_RELEASE_FLAGS "-O3", "-DNDEBUG", "-DZOD_CONSOLE_ENABLE=0"

#ifdef _WIN32
#define EXE_EXT    ".exe"
#define BIN_PREFIX ""
#else
#define EXE_EXT    ""
#define BIN_PREFIX "./"
#endif

#define C_ENTRY "main.c"

typedef struct {
    const char *name;        // CLI target name
    const char *entry;       // source file compiled as the single TU
    const char *output;      // bare binary name, no "./" prefix, no EXE_EXT
    bool        needs_math;  // -lm
} build_target;

static const build_target BUILD_TARGETS[] = {
     {"main", "main.c", "main", false},
     {"engine", "ngine.example/engine_run_example.c", "engine_run", true},
};
static const size_t BUILD_TARGETS_COUNT =
     sizeof(BUILD_TARGETS) / sizeof(BUILD_TARGETS[0]);

static const build_target *find_target(const char *name) {
    for (size_t i = 0; i < BUILD_TARGETS_COUNT; ++i)
        if (strcmp(BUILD_TARGETS[i].name, name) == 0) return &BUILD_TARGETS[i];
    return NULL;
}

static const char *target_bin_path(const build_target *t) {
    return nob_temp_sprintf("%s%s%s", BIN_PREFIX, t->output, EXE_EXT);
}

#define GLAD_SRC "thirdparty/glad/src/gl.c"

#ifdef _WIN32
#define SDL_FLAGS    "-I/ucrt64/include/SDL3", "-L/ucrt64/lib", "-lSDL3.dll"
#define GLAD_FLAGS   "-Ithirdparty/glad/include"
#define VULKAN_FLAGS "-lvulkan-1"
#define MATH_FLAGS   "-lm"
#elif defined(__linux__)
#define SDL_FLAGS    "-I/usr/include/SDL3", "-lSDL3"
#define GLAD_FLAGS   "-Ithirdparty/glad/include", "-ldl"
#define VULKAN_FLAGS "-lvulkan"
#define MATH_FLAGS   "-lm"
#endif

#define RENDER_BACKEND_OPENGL_DEFINE "-DRENDER_BACKEND=RENDER_BACKEND_OPENGL"
#define RENDER_BACKEND_VULKAN_DEFINE "-DRENDER_BACKEND=RENDER_BACKEND_VULKAN"

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
        nob_cmd_append(&cmd, C_COMPILER, C_FLAGS, C_DEBUG_FLAGS, "-o", bin, src,
                       MATH_FLAGS);
        if (!nob_cmd_run(&cmd)) return 1;
    }
    return 0;
}

int run_test_dir(bool asan, const char *dir, bool needs_gl_sdl, int *passed,
                 int *failed) {
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
        if (needs_gl_sdl) nob_cmd_append(&test_cmd, GLAD_SRC, SDL_FLAGS, GLAD_FLAGS);
        nob_cmd_append(&test_cmd, MATH_FLAGS);
        if (asan) nob_cmd_append(&test_cmd, C_ASAN_FLAGS);
        if (!nob_cmd_run(&test_cmd)) return 1;

        Nob_Cmd run_test = {0};
        nob_cmd_append(&run_test, bin);
        if (nob_cmd_run(&run_test)) {
            nob_log(NOB_INFO, "PASS  %s/%s", dir, name);
            (*passed)++;
        } else {
            nob_log(NOB_ERROR, "FAIL  %s/%s", dir, name);
            (*failed)++;
        }
    }
    return 0;
}

typedef struct {
    const char *dir;
    bool        needs_gl_sdl;
} test_dir_entry;

static const test_dir_entry TEST_DIRS[] = {
     {"ngine.lib", false},
     {"ngine.lib/collections", false},
     {"ngine.core/test", true},
     {"ngine.ext.console/test", true},
};
static const size_t TEST_DIRS_COUNT = sizeof(TEST_DIRS) / sizeof(TEST_DIRS[0]);

static int check_modules_separation(void) {
    static const char *const dirs[] = {"ngine.lib", "ngine.lib/collections"};
    bool                     clean  = true;

    for (size_t d = 0; d < sizeof(dirs) / sizeof(*dirs); ++d) {
        Nob_File_Paths files = {0};
        if (!nob_read_entire_dir(dirs[d], &files)) return 1;

        for (size_t i = 0; i < files.count; ++i) {
            const char *name = files.items[i];
            size_t      len  = strlen(name);
            if (len < 2 || strcmp(name + len - 2, ".h") != 0) continue;

            const char        *path = nob_temp_sprintf("%s/%s", dirs[d], name);
            Nob_String_Builder sb   = {0};
            if (!nob_read_entire_file(path, &sb)) return 1;
            nob_sb_append_null(&sb);

            char *line = strtok(sb.items, "\n");
            while (line) {
                if (strstr(line, "#include") && strstr(line, "ngine")) {
                    nob_log(NOB_ERROR, "check-modules: %s: %s", path, line);
                    clean = false;
                }
                line = strtok(NULL, "\n");
            }
            nob_da_free(sb);
        }
    }

    if (clean) nob_log(NOB_INFO, "check-modules: OK");
    return clean ? 0 : 1;
}

int run_tests(bool asan, const char *dir) {
#ifdef _WIN32
    if (!nob_mkdir_if_not_exists("tmp")) return 1;
#endif
    if (check_modules_separation() != 0) return 1;
    int passed = 0, failed = 0;
    if (dir) {
        bool found = false;
        for (size_t i = 0; i < TEST_DIRS_COUNT; ++i) {
            if (strcmp(dir, TEST_DIRS[i].dir) == 0 ||
                nob_sv_starts_with(nob_sv_from_cstr(TEST_DIRS[i].dir),
                                   nob_sv_from_cstr(dir))) {
                run_test_dir(asan, TEST_DIRS[i].dir, TEST_DIRS[i].needs_gl_sdl, &passed,
                             &failed);
                found = true;
            }
        }
        if (!found) {
            nob_log(NOB_ERROR,
                    "unknown test dir '%s'. use: ./nob test ngine.lib | ./nob test ngine",
                    dir);
            return 1;
        }
    } else {
        for (size_t i = 0; i < TEST_DIRS_COUNT; ++i)
            run_test_dir(asan, TEST_DIRS[i].dir, TEST_DIRS[i].needs_gl_sdl, &passed,
                         &failed);
    }
    nob_log(failed ? NOB_ERROR : NOB_INFO,
            "\n────────────────────────────────────\n"
            "[test] %d passed  %d failed\n"
            "────────────────────────────────────",
            passed, failed);
    return failed > 0 ? 1 : 0;
}

int run_clean(void) {
    for (size_t i = 0; i < TEST_DIRS_COUNT; ++i) {
        Nob_File_Paths files = {0};
        if (!nob_read_entire_dir(TEST_DIRS[i].dir, &files)) continue;
        for (size_t j = 0; j < files.count; ++j) {
            const char *name = files.items[j];
            if (!nob_sv_starts_with(nob_sv_from_cstr(name), nob_sv_from_cstr("test_")))
                continue;
            const char *bin =
                 nob_temp_sprintf("./%.*s.out%s", (int)(strlen(name) - 2), name, EXE_EXT);
            if (nob_file_exists(bin) > 0) nob_delete_file(bin);
        }
    }
    for (size_t i = 0; i < BUILD_TARGETS_COUNT; ++i) {
        const char *bin = target_bin_path(&BUILD_TARGETS[i]);
        if (nob_file_exists(bin) > 0) nob_delete_file(bin);
    }
    return 0;
}

int run_run(bool execute, const char *target_name, const char *mode,
            const char *backend) {
    if (strcmp(mode, "debug") != 0 && strcmp(mode, "release") != 0) {
        nob_log(NOB_ERROR, "unknown build mode '%s'. use: debug | release", mode);
        return 1;
    }
    if (strcmp(backend, "opengl") != 0 && strcmp(backend, "vulkan") != 0) {
        nob_log(NOB_ERROR, "unknown render backend '%s'. use: opengl | vulkan", backend);
        return 1;
    }
    const build_target *target = find_target(target_name);
    if (!target) {
        nob_log(NOB_ERROR, "unknown target '%s'", target_name);
        return 1;
    }
    bool        is_opengl = strcmp(backend, "opengl") == 0;
    const char *out       = target_bin_path(target);
    const char *src       = target->entry;
    const char *backend_de =
         is_opengl ? RENDER_BACKEND_OPENGL_DEFINE : RENDER_BACKEND_VULKAN_DEFINE;

    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, C_COMPILER, C_FLAGS, "-o", out, src, backend_de, SDL_FLAGS);
    if (is_opengl) {
        nob_cmd_append(&cmd, GLAD_SRC, GLAD_FLAGS);
    } else {
        nob_cmd_append(&cmd, VULKAN_FLAGS);
    }
    if (target->needs_math) nob_cmd_append(&cmd, MATH_FLAGS);
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

static const char *COMPDB_SCAN_DIRS[] = {
     "ngine.lib",
     "ngine.lib/collections",
     "ngine.core",
     "ngine.core/test",
     "ngine.core/internal/clock",
     "ngine.core/internal/config",
     "ngine.core/internal/engine_context",
     "ngine.core/internal/input",
     "ngine.core/internal/window",
     "ngine.core/internal/render",
     "ngine.core/internal/zod_ngine",
     "ngine.ext.console",
     "ngine.ext.console/internal/console",
     "ngine.ext.console/test",
     "ngine.core/internal/error",
     "ngine.example",
};
static const size_t COMPDB_SCAN_DIRS_COUNT =
     sizeof(COMPDB_SCAN_DIRS) / sizeof(COMPDB_SCAN_DIRS[0]);

static const char  *COMPDB_DEFINES[] = {"-DCARG_IMPLEMENTATION",
                                        "-DCVAR_IMPLEMENTATION",
                                        "-DCVAR_LOAD_IMPLEMENTATION",
                                        "-DINI_IMPLEMENTATION",
                                        "-DSCF_IMPLEMENTATION",
                                        "-DLOG_IMPLEMENTATION",
                                        "-DLOG_USE_SIMPLE",
                                        "-DFILE_WATCHER_IMPLEMENTATION",
                                        "-DSTRING_VIEW_IMPLEMENTATION",
                                        "-DARRAY_LIST_IMPLEMENTATION",
                                        "-DTYPES_IMPLEMENTATION",
                                        "-DZOD_NGINE_IMPLEMENTATION",
                                        "-DNOB_IMPLEMENTATION"};
static const size_t COMPDB_DEFINES_COUNT =
     sizeof(COMPDB_DEFINES) / sizeof(COMPDB_DEFINES[0]);

static const char *json_path(const char *path) {
    char *out = nob_temp_alloc(strlen(path) + 1);
    for (size_t i = 0; path[i]; ++i) out[i] = path[i] == '\\' ? '/' : path[i];
    out[strlen(path)] = '\0';
    return out;
}

static void compdb_entry(FILE *f, const char *cwd, const char *filepath, bool is_header,
                         bool *first) {
    if (!*first) fprintf(f, ",\n");
    *first = false;
    fprintf(f, "  {\n");
    const char *json_cwd = json_path(cwd);
    fprintf(f, "    \"directory\": \"%s\",\n", json_cwd);
    fprintf(f, "    \"file\": \"%s/%s\",\n", json_cwd, filepath);
    fprintf(f, "    \"arguments\": [\n");
    fprintf(f, "      \"cc\", \"-Wall\", \"-Wextra\", \"-std=c23\", \"-I.\",\n");
#ifdef __linux__
    fprintf(f, "      \"-I/usr/include/SDL3\",\n");
#elif defined(_WIN32)
    fprintf(f, "      \"-I/ucrt64/include/SDL3\",\n");
#endif
    fprintf(f, "      \"-Ithirdparty/glad/include\",\n");
    for (size_t i = 0; i < COMPDB_DEFINES_COUNT; ++i)
        fprintf(f, "      \"%s\",\n", COMPDB_DEFINES[i]);
    if (is_header) fprintf(f, "      \"-x\", \"c\",\n");
    fprintf(f, "      \"%s/%s\"\n", json_cwd, filepath);
    fprintf(f, "    ]\n");
    fprintf(f, "  }");
}

int run_compdb(void) {
    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd))) {
        nob_log(NOB_ERROR, "getcwd failed");
        return 1;
    }

    FILE *f = fopen("compile_commands.json", "w");
    if (!f) {
        nob_log(NOB_ERROR, "failed to open compile_commands.json");
        return 1;
    }

    fprintf(f, "[\n");
    bool first = true;

    for (size_t d = 0; d < COMPDB_SCAN_DIRS_COUNT; ++d) {
        const char    *dir   = COMPDB_SCAN_DIRS[d];
        Nob_File_Paths files = {0};
        if (!nob_read_entire_dir(dir, &files)) continue;
        for (size_t i = 0; i < files.count; ++i) {
            const char *name = files.items[i];
            size_t      len  = strlen(name);
            bool        is_c = len > 2 && strcmp(name + len - 2, ".c") == 0;
            bool        is_h = len > 2 && strcmp(name + len - 2, ".h") == 0;
            if (!is_c && !is_h) continue;
            compdb_entry(f, cwd, nob_temp_sprintf("%s/%s", dir, name), is_h, &first);
        }
    }

    if (nob_file_exists(C_ENTRY) > 0) compdb_entry(f, cwd, C_ENTRY, false, &first);

    fprintf(f, "\n]\n");
    fclose(f);
    nob_log(NOB_INFO, "wrote compile_commands.json (%zu dirs scanned)",
            COMPDB_SCAN_DIRS_COUNT);
    return 0;
}

static const char *parse_backend_flag(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--backend=", 10) == 0) return argv[i] + 10;
    }
    return "opengl";
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (argc > 1 && strcmp(argv[1], "help") == 0) {
        nob_log(NOB_INFO, "usage: ./nob [command] [options]");
        nob_log(NOB_INFO, "  (none)                        build main debug");
        nob_log(NOB_INFO, "  run [engine] [debug|release] [--backend=opengl|vulkan]");
        nob_log(NOB_INFO, "                                build and run");
        nob_log(NOB_INFO, "  build-debug [engine] [--backend=opengl|vulkan]");
        nob_log(NOB_INFO, "                                build with debug symbols");
        nob_log(NOB_INFO, "  build-release [engine] [--backend=opengl|vulkan]");
        nob_log(NOB_INFO, "                                build with optimizations");
        nob_log(NOB_INFO, "  test [dir]                    run tests (ngine.lib, ngine)");
        nob_log(NOB_INFO, "  test-asan [dir]               run tests with asan");
        nob_log(NOB_INFO, "  clean                         remove build artifacts");
        nob_log(NOB_INFO,
                "  compdb                        generate compile_commands.json");
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "compdb") == 0) {
        return run_compdb();
    }

    if (argc > 1 && strcmp(argv[1], "test-build") == 0) {
        const char *dir = argc > 2 ? argv[2] : NULL;
        if (dir) return build_test_dir(dir);
        for (size_t i = 0; i < TEST_DIRS_COUNT; ++i) build_test_dir(TEST_DIRS[i].dir);
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
        return run_run(true, engine ? "engine" : "main", mode,
                       parse_backend_flag(argc, argv));
    }

    if (argc > 1 &&
        (strcmp(argv[1], "build-debug") == 0 || strcmp(argv[1], "build-release") == 0)) {
        bool        engine = argc > 2 && strcmp(argv[2], "engine") == 0;
        const char *mode   = strcmp(argv[1], "build-release") == 0 ? "release" : "debug";
        return run_run(false, engine ? "engine" : "main", mode,
                       parse_backend_flag(argc, argv));
    }

    if (argc > 1) {
        nob_log(NOB_ERROR, "unknown command '%s'. run ./nob help", argv[1]);
        return 1;
    }
    return run_run(false, "main", "debug", "opengl");
}
