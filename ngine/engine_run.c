#include <stdbool.h>

#define ZOD_NGINE_IMPLEMENTATION
#include "index.h"

void before_init(void) {
    log_set_level(LOG_TRACE);
    log_debug("Before init");
}

bool get_args(const int argc, const char **argv, carg_table *cargs) {
    carg_register_t defs[] = {
         {.flag = "--debug-log", .arg_count = 0, .type = CARG_BOOL, .required = false},
         {.flag = "--size", .arg_count = 2, .type = CARG_INT, .required = false},
    };

    return carg_parse(defs, 2, argc, argv, cargs);
}

void after_init(void) { log_debug("After init"); }

int main(const int argc, const char **argv) {
    const zod_engine_dispatch dispatch = {
         .before_init = before_init, .get_args = get_args, .after_init = after_init};

    const zod_engine_init_params params = {
         .argc = argc, .argv = argv, .dispatch = dispatch};

    zod_ngine_init(params);
    zod_ngine_destroy();
    return 0;
}
