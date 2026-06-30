# zod-ngine

Game engine written in C23 using SDL3 and OpenGL 4.6.

## Stack

- **Language:** C23, compiled with `clang`
- **Windowing/Input:** SDL3
- **Rendering:** OpenGL 4.6 core profile (via glad)
- **Build:** `nob.h` (single-header build system)

## Structure

```
zod-ngine/
├── modules/          single-header utility libraries
│   ├── ini.h         INI/SCF config parser
│   ├── cvar.h        config variable store
│   ├── log.h         logging
│   └── file_watcher.h
├── ngine/            engine layer (unity build via ngine/index.h)
│   ├── zod_ngine.h   public API
│   ├── internal/     subsystems (clock, config, input, console, window)
│   └── test/         engine unit tests
├── lib/              vendored headers (nob.h, glad, minunit.h) — do not edit
├── resources/        learning material and assets
└── build.c           build configuration
```

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
