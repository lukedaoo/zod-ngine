cmake_minimum_required(VERSION 3.23)
project(zod_ngine C)

set(CMAKE_C_STANDARD 23)

include_directories(lib)
include_directories(modules)
include_directories(ngine)

find_package(PkgConfig REQUIRED)

file(GLOB MODULE_HEADERS         CONFIGURE_DEPENDS "modules/*.h")
file(GLOB MODULE_COLL_HEADERS    CONFIGURE_DEPENDS "modules/collections/*.h")
file(GLOB NGINE_HEADERS          CONFIGURE_DEPENDS "ngine/*.h")
file(GLOB NGINE_INTERNAL_SOURCES CONFIGURE_DEPENDS "ngine/internal/*.c")

add_executable(zod_ngine
    ${MODULE_HEADERS}
    ${MODULE_COLL_HEADERS}
    ${NGINE_HEADERS}
    ${NGINE_INTERNAL_SOURCES}

    lib/minunit.h
    lib/nob.h

    main.c
)
