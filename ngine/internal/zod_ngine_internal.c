#ifdef ZOD_NGINE_IMPLEMENTATION
#include <assert.h>
#include <stdio.h>

#include "../../modules/carg.h"
#include "../../modules/log.h"

#include "../engine_context.h"
#include "../zod_ngine.h"

struct zod_engine_dispatch {
    void (*before_init)(void);
    bool (*load_config_from_file)(const char *filename);
    bool (*get_args)(const int argc, const char **argv, carg_table *cargs);
    void (*after_init)(void);

    void (*before_destroy)(void);
    void (*after_destroy)(void);
};

struct zod_engine_init_params {
    int                 argc;
    const char        **argv;
    zod_engine_dispatch dispatch;
};

bool zod_ngine_init(const zod_engine_init_params params) {
    const int                 argc     = params.argc;
    const char              **argv     = params.argv;
    const zod_engine_dispatch dispatch = params.dispatch;
    if (dispatch.before_init) {
        dispatch.before_init();
    }

    log_debug("initializing engine...");

    //
    // @todo: load preset config
    //
    {
        log_debug("loading preset config");
    }

    //
    // @todo: load config file if it exists
    //
    {
        if (dispatch.load_config_from_file) {
            log_debug("loading config file");
        }
    }

    //
    // parse command line
    //
    {
        carg_table cargs = {0};
        if (dispatch.get_args) {
            log_debug("parsing command line...");
            if (!dispatch.get_args(argc, argv, &cargs)) {
                log_debug("failed to parse command line");
            }
        }
    }

    //
    // @todo: finalize the engine config (cvars from presets, config file, command line)
    //
    {
        log_debug("finalizing engine config");
    }

    g_core.config = NULL;

    if (dispatch.after_init) {
        dispatch.after_init();
    }

    return true;
}

void zod_ngine_destroy(void) {
    // if (dispatch.before_destroy) {
    //     dispatch.before_destroy();
    // }

    //
    // Destroy engine context
    //
    log_debug("destroying engine...");

    // if (dispatch.after_destroy) {
    //     dispatch.after_destroy();
    // }
}

#endif
