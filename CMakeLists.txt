cmake_minimum_required(VERSION 4.3)
project(zod_ngine C)

set(CMAKE_C_STANDARD 23)

include_directories(lib)
include_directories(modules)

add_executable(zod_ngine
        lib/minunit.h
        lib/nob.h

        modules/cvar.h
        modules/cvar_load.h
        modules/file_watcher.h
        modules/index.h
        modules/ini.h
        modules/log.h
        modules/scf.h
        modules/string_view.h

        modules/test_cvar.c
        modules/test_cvar_load.c
        modules/test_file_watcher.c
        modules/test_ini.c
        modules/test_scf.c
        modules/test_string_view.c

        build.c
        main.c)
