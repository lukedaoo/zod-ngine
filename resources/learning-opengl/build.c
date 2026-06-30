#define NOB_IMPLEMENTATION
#include "../../lib/nob.h"

#define CC      "cc"
#define CFLAGS  "-std=c23", "-Wall", "-Wextra", "-I../../lib/glad/include"
#define GLAD    "../../lib/glad/src/gl.c"
#define LDFLAGS "-lglfw", "-ldl"

#define SRC_DIR "src"
#define BIN_DIR "bin"

int build_all(void) {
    if (!nob_mkdir_if_not_exists(BIN_DIR)) return 1;

    Nob_File_Paths files = {0};
    if (!nob_read_entire_dir(SRC_DIR, &files)) return 1;

    int failed = 0;
    for (size_t i = 0; i < files.count; ++i) {
        const char *name = files.items[i];
        size_t      len  = strlen(name);
        if (len < 2 || strcmp(name + len - 2, ".c") != 0) continue;

        const char *src = nob_temp_sprintf("%s/%s", SRC_DIR, name);
        const char *bin = nob_temp_sprintf("%s/%.*s", BIN_DIR, (int)(len - 2), name);

        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, CC, CFLAGS, "-o", bin, src, GLAD, LDFLAGS);
        if (!nob_cmd_run(&cmd)) failed++;
    }
    return failed > 0 ? 1 : 0;
}

int clean(void) {
    Nob_File_Paths files = {0};
    if (!nob_read_entire_dir(BIN_DIR, &files)) return 0;
    for (size_t i = 0; i < files.count; ++i) {
        const char *name = files.items[i];
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
        nob_delete_file(nob_temp_sprintf("%s/%s", BIN_DIR, name));
    }
    return 0;
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (argc > 1 && strcmp(argv[1], "clean") == 0) return clean();

    if (argc > 1 && strcmp(argv[1], "help") == 0) {
        nob_log(NOB_INFO, "usage: ./nob [command]");
        nob_log(NOB_INFO, "  (none)   build all examples");
        nob_log(NOB_INFO, "  clean    remove bin/");
        return 0;
    }

    return build_all();
}
