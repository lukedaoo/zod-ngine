cmake_minimum_required(VERSION 3.23)
project(zod_ngine C)

set(CMAKE_C_STANDARD 23)

include_directories(thirdparty)
include_directories(ngine.lib)
include_directories(ngine.core)
include_directories(ngine.ext.console)
include_directories(ngine.example)

find_package(PkgConfig REQUIRED)

file(GLOB MODULE_HEADERS         CONFIGURE_DEPENDS "ngine.lib/*.h")
file(GLOB MODULE_COLL_HEADERS    CONFIGURE_DEPENDS "ngine.lib/collections/*.h")
file(GLOB NGINE_HEADERS          CONFIGURE_DEPENDS "ngine.core/*.h")
file(GLOB NGINE_INTERNAL_SOURCES CONFIGURE_DEPENDS "ngine.core/internal/*.c")
file(GLOB CONSOLE_HEADERS        CONFIGURE_DEPENDS "ngine.ext.console/*.h")

add_executable(zod_ngine
    ${MODULE_HEADERS}
    ${MODULE_COLL_HEADERS}
    ${NGINE_HEADERS}
    ${NGINE_INTERNAL_SOURCES}
    ${CONSOLE_HEADERS}

    thirdparty/minunit.h
    thirdparty/nob.h

    ngine.example/engine_run_example.c
)
