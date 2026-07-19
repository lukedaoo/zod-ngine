# zod-ngine

> **Status:** In Progress

Game engine written in C23 using SDL3 and OpenGL 4.6.

## Stack

- **Language:** C23, compiled with `clang`
- **Windowing/Input:** SDL3
- **Rendering:** OpenGL 4.6 core profile (via glad)
- **Build:** `nob.h` (single-header build system)

## Structure

```
zod-ngine/
├── ngine.lib/            single-header utility libraries
│   ├── ini.h             INI/SCF config parser
│   ├── cvar.h            config variable store
│   ├── log.h             logging
│   ├── types.h           u8..f64, color4f
│   └── file_watcher.h
├── ngine.core/           engine layer (unity build via ngine.core/index.h)
│   ├── zod_ngine.h       public API + zod_extension registry
│   ├── internal/         subsystems (clock, config, input, window, render)
│   └── test/             engine unit tests — link with zero extensions
├── ngine.ext.console/    in-game console, an optional extension
│   ├── console.h         public API + console_ext_install()
│   ├── internal/console/ console state + OpenGL rendering
│   └── test/             console unit tests
├── ngine.example/        runnable example app (links core + console)
│   └── engine_run_example.c
├── thirdparty/           vendored headers (nob.h, glad, minunit.h, stb_truetype.h) — do not edit
├── resources/            learning material and assets
└── build.c               build configuration
```

Core never includes or calls into an extension by name — extensions
register generic config/apply hooks via `zod_register_extension`, and the
application (not core) wires them in. See `CONTEXT.md` for details.

## Build

```sh
cc -o nob build.c && ./nob   # bootstrap + build (debug)

./nob run engine              # build and run engine
./nob run engine release      # release build
./nob test                    # run all tests
./nob test-asan               # tests with address sanitizer
./nob help                    # full command reference
```

## Motivation

Building a game engine from scratch, using AI as a force multiplier. Every feature, architecture decision, and design choice is envisioned by me — AI accelerates the implementation. The goal is to make a game engine works at every level, without hiding behind existing frameworks.
