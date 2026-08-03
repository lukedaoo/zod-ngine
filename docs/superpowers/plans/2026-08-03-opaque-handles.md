# Opaque Handles Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace every public-facing `command *`, `action *`, and event-listener pointer in zod-ngine with an opaque `unsigned int` handle, so no public header exposes the layout of those records.

**Architecture:** Each of the three single-header modules (`ngine.lib/command.h`, `ngine.lib/event.h`, `ngine.lib/action.h`) gains a `typedef unsigned int *_handle` where `0` is the invalid handle. A handle encodes the record's index in the module's backing `array_list` as `index + 1`; `command` additionally packs its `command_group` into the low bit. A private `static` resolver per module converts handle to record pointer and returns `NULL` for `0`, out-of-range, or `NULL` table. The value types that public signatures use by value (`action_execute_result`, `action_executor`, `event_callback_result`, `event_context`, `event_identifier`) move from the implementation section to the public section; the identity types (`struct command`, `struct action`, `struct action_binding`, `event_listener`) stay private.

**Tech Stack:** C23, single-header STB-style libraries guarded by `*_IMPLEMENTATION` macros, `array_list` from `ngine.lib/collections/array_list.h`, minunit (`thirdparty/minunit.h`) for tests, `nob` build system driven through the `Makefile`.

**Spec:** `docs/superpowers/specs/2026-08-03-opaque-handles-design.md`

## Global Constraints

- Handle types are exactly `typedef unsigned int command_handle;`, `typedef unsigned int event_handle;`, `typedef unsigned int action_handle;`.
- The invalid handle is `0u` for all three, exposed as `COMMAND_HANDLE_INVALID`, `EVENT_HANDLE_INVALID`, `ACTION_HANDLE_INVALID`.
- Because handles are unsigned, invalid-handle guards use `handle == 0` — never a sign test such as `handle <= 0`.
- **No attribute accessors.** Do not add `command_get_name`, `command_get_group`, `action_get_name`, `action_get_key`, `action_get_context`, `action_get_type`, or any equivalent getter. A handle is an identity token only.
- No assertions and no aborts on invalid input. Every public entry point returns the documented error value: `*_HANDLE_INVALID` for lookups and registration, the module's existing not-found or VOID result for `*_execute`.
- Public section of each of the three headers is the text **before** `#ifdef COMMAND_IMPLEMENTATION` / `#ifdef EVENT_IMPLEMENTATION` / `#ifdef ACTION_IMPLEMENTATION`. It must contain no definition of `struct command`, `struct action`, `struct action_binding`, or `event_listener`, and no function signature naming those types.
- Resolvers are `static` and live inside the implementation block. They are never declared in the public section.
- Existing code style: 4-space indent, `.clang-format` is present — run nothing manually, just match surrounding formatting.
- Tests use minunit (`MU_TEST`, `MU_TEST_SUITE`, `MU_RUN_TEST`, `mu_check`, `mu_assert_int_eq`).
- Build and test only via the `Makefile` wrappers or `./nob` directly. Test binaries land in the repo root as `test_<name>.out`.

## Build & Test Commands

| Purpose | Command |
| --- | --- |
| Single lib test | `make test ngine.lib/test_command.c` |
| All `ngine.lib` tests | `make test ngine.lib` |
| All `ngine.core` tests | `make test ngine.core/test` |
| All `ngine.ext.console` tests | `make test ngine.ext.console/test` |
| Whole suite | `make test` |
| Whole suite under ASan | `make test-asan` |
| Build the engine binary | `make build-debug` |
| Build the beatup binary | `make build-debug beatup` |

`make test` also runs `check_modules_separation()`, which fails the run if any header under `ngine.lib/` or `ngine.lib/collections/` `#include`s an `ngine`-prefixed path. Do not add such includes to the three module headers.

## File Structure

**Created**

| Path | Responsibility |
| --- | --- |
| `tools/check_public_boundary.sh` | Greps the public section of the three module headers and fails if a private struct or a forbidden type name appears. Run by hand and by the final task. |

**Modified**

| Path | Change |
| --- | --- |
| `ngine.lib/command.h` | `command_handle`, group-packed encoding, resolver, handle-returning register/get, table-taking `command_execute` |
| `ngine.lib/event.h` | `event_handle` for subscriptions, public value types, handle-based unsubscribe |
| `ngine.lib/action.h` | `action_handle`, public value types, new `action_exec_fn` signature, table-taking `action_execute` |
| `ngine.lib/test_command.c` | Ported to handles, plus new handle-contract tests |
| `ngine.lib/test_event.c` | Ported to subscription handles, plus new handle-contract tests |
| `ngine.lib/test_action.c` | Ported to handles and the new executor signature, plus new handle-contract tests |
| `ngine.core/cmd_manager.h` | `cmd_manager_priv_register` returns `command_handle` |
| `ngine.core/internal/cmd_manager/cmd_manager.c` | Same, plus removes the `mgr->table.system_commands.header.size` assertion |
| `ngine.core/event_manager.h` | `event_manager_priv_subscribe` returns `event_handle`; unsubscribe takes a handle |
| `ngine.core/internal/event_manager/event_manager.c` | Same |
| `ngine.core/action_manager.h` | Resolves and binds to `action_handle`; execute takes manager + handle |
| `ngine.core/internal/action_manager/action_manager.c` | Same |
| `ngine.core/zod_ngine.h` | `zngine_command_register`, `zngine_event_subscribe`, `zngine_event_unsubscribe`, `zngine_action_*` switch to handles |
| `ngine.core/internal/zod_ngine/zod_ngine.c` | Same for command and event wrappers |
| `ngine.core/internal/zod_ngine/zod_ngine_action.c` | Same for action wrappers |
| `ngine.core/test/test_cmd_manager.c` | Ported off `command_table_get(...) != NULL` and off `mgr.table` |
| `ngine.example/engine_run_example.c` | New executor signature; handle-typed subscribe/bind returns |
| `ngine.example/beatup.c` | Same, if it binds actions or subscribes |

`ngine.ext.console/internal/console/console.c` and `ngine.ext.console/test/test_console.c` use only the name-based command API, which is unchanged. They are verified, not edited.

## Task Ordering

The three modules are changed one at a time, and each module's `ngine.lib` change is immediately followed by the `ngine.core` change that consumes it. That keeps `make test` green at the end of every even-numbered task. Tasks 1, 3, and 5 leave `ngine.core` temporarily broken and therefore verify with a **single-file** test command, not the whole suite.

---

### Task 1: command.h — handles

**Files:**
- Modify: `ngine.lib/command.h`
- Test: `ngine.lib/test_command.c`

**Interfaces:**
- Consumes: nothing from earlier tasks.
- Produces:
  - `typedef unsigned int command_handle;` and `#define COMMAND_HANDLE_INVALID 0u`
  - `command_handle command_table_register(command_table *table, command_group group, const char *name, command_execute_result (*handler)(int argc, char **argv));`
  - `command_handle command_table_get(const command_table *table, const command_group group, const char *name);`
  - `command_execute_result command_execute(const command_table *table, command_handle handle, int argc, char **argv);`
  - Unchanged: `command_table_init`, `command_table_init_with_capacity`, `command_table_destroy`, `command_table_reserve_system_commands`, `command_table_reserve_user_defined_commands`, `command_table_unregister`, `command_execute_by_name`.

- [ ] **Step 1: Write the failing tests**

Append to `ngine.lib/test_command.c`, immediately before the final `main` function:

```c
MU_TEST(test_handle_register_returns_valid_handle) {
    command_table table;
    command_table_init(&table);
    command_handle h = command_table_register(&table, COMMAND_GROUP_SYSTEM, "noop", noop);
    mu_check(h != COMMAND_HANDLE_INVALID);
    command_table_destroy(&table);
}

MU_TEST(test_handle_register_duplicate_returns_invalid) {
    command_table table;
    command_table_init(&table);
    command_table_register(&table, COMMAND_GROUP_SYSTEM, "noop", noop);
    command_handle dup = command_table_register(&table, COMMAND_GROUP_SYSTEM, "noop", noop);
    mu_check(dup == COMMAND_HANDLE_INVALID);
    command_table_destroy(&table);
}

MU_TEST(test_handle_get_matches_register) {
    command_table table;
    command_table_init(&table);
    command_handle registered =
         command_table_register(&table, COMMAND_GROUP_SYSTEM, "noop", noop);
    command_handle looked_up = command_table_get(&table, COMMAND_GROUP_SYSTEM, "noop");
    mu_check(registered == looked_up);
    command_table_destroy(&table);
}

MU_TEST(test_handle_get_missing_returns_invalid) {
    command_table table;
    command_table_init(&table);
    mu_check(command_table_get(&table, COMMAND_GROUP_SYSTEM, "nope") ==
             COMMAND_HANDLE_INVALID);
    command_table_destroy(&table);
}

MU_TEST(test_handle_round_trip_execute) {
    command_table table;
    command_table_init(&table);
    command_table_register(&table, COMMAND_GROUP_SYSTEM, "pi", pi_handler);
    command_handle         h   = command_table_get(&table, COMMAND_GROUP_SYSTEM, "pi");
    command_execute_result res = command_execute(&table, h, 0, NULL);
    mu_assert_int_eq(COMMAND_RESULT_FLOAT, res.type);
    command_table_destroy(&table);
}

MU_TEST(test_handle_zero_is_invalid) {
    command_table  table;
    command_handle zero = 0;
    command_table_init(&table);
    command_table_register(&table, COMMAND_GROUP_SYSTEM, "noop", noop);
    command_execute_result res = command_execute(&table, zero, 0, NULL);
    mu_assert_int_eq(COMMAND_RESULT_COMMAND_NOT_FOUND, res.type);
    command_table_destroy(&table);
}

MU_TEST(test_handle_out_of_range_is_invalid) {
    command_table table;
    command_table_init(&table);
    command_table_register(&table, COMMAND_GROUP_SYSTEM, "noop", noop);
    command_execute_result res = command_execute(&table, 9999u, 0, NULL);
    mu_assert_int_eq(COMMAND_RESULT_COMMAND_NOT_FOUND, res.type);
    command_table_destroy(&table);
}

MU_TEST(test_handle_encodes_group) {
    command_table table;
    command_table_init(&table);
    command_handle sys =
         command_table_register(&table, COMMAND_GROUP_SYSTEM, "dup", pi_handler);
    command_handle usr =
         command_table_register(&table, COMMAND_GROUP_USER_DEFINED, "dup", noop);
    mu_check(sys != usr);
    mu_assert_int_eq(COMMAND_RESULT_FLOAT, command_execute(&table, sys, 0, NULL).type);
    mu_assert_int_eq(COMMAND_RESULT_VOID, command_execute(&table, usr, 0, NULL).type);
    command_table_destroy(&table);
}

MU_TEST(test_handle_invalidated_by_unrelated_removal) {
    command_table table;
    command_table_init(&table);
    command_table_register(&table, COMMAND_GROUP_SYSTEM, "first", noop);
    command_handle second_before =
         command_table_register(&table, COMMAND_GROUP_SYSTEM, "second", noop);
    command_table_unregister(&table, COMMAND_GROUP_SYSTEM, "first");
    command_handle second_after = command_table_get(&table, COMMAND_GROUP_SYSTEM, "second");
    // documented behaviour: removal compacts storage, so earlier handles shift
    mu_check(second_before != second_after);
    mu_check(second_after != COMMAND_HANDLE_INVALID);
    command_table_destroy(&table);
}

MU_TEST_SUITE(command_handle_suite) {
    MU_RUN_TEST(test_handle_register_returns_valid_handle);
    MU_RUN_TEST(test_handle_register_duplicate_returns_invalid);
    MU_RUN_TEST(test_handle_get_matches_register);
    MU_RUN_TEST(test_handle_get_missing_returns_invalid);
    MU_RUN_TEST(test_handle_round_trip_execute);
    MU_RUN_TEST(test_handle_zero_is_invalid);
    MU_RUN_TEST(test_handle_out_of_range_is_invalid);
    MU_RUN_TEST(test_handle_encodes_group);
    MU_RUN_TEST(test_handle_invalidated_by_unrelated_removal);
}
```

Add `MU_RUN_SUITE(command_handle_suite);` to the `main` function alongside the existing `MU_RUN_SUITE` calls.

`noop` and `pi_handler` are existing handlers in this file. Open the file and confirm their exact names before writing the tests; if they differ, use the file's actual names — `noop` must return `COMMAND_RESULT_VOID` and `pi_handler` must return `COMMAND_RESULT_FLOAT`. If no such pair exists, add them at the top of the file:

```c
static command_execute_result noop(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return (command_execute_result){.type = COMMAND_RESULT_VOID};
}

static command_execute_result pi_handler(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return (command_execute_result){.type = COMMAND_RESULT_FLOAT, .value.f = 3.14f};
}
```

- [ ] **Step 2: Run the tests to verify they fail**

```bash
make test ngine.lib/test_command.c
```

Expected: compilation failure — `unknown type name 'command_handle'`.

- [ ] **Step 3: Rewrite the public section of `ngine.lib/command.h`**

Replace lines 1–52 of `ngine.lib/command.h` (everything from `#ifndef COMMAND_H` down to and including the `command_execute` declaration, i.e. everything before `#ifdef COMMAND_IMPLEMENTATION`) with:

```c
#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>
#include <stdbool.h>

typedef enum { COMMAND_GROUP_SYSTEM, COMMAND_GROUP_USER_DEFINED } command_group;

typedef enum {
    COMMAND_RESULT_VOID,
    COMMAND_RESULT_INT,
    COMMAND_RESULT_FLOAT,
    COMMAND_RESULT_STRING,
    COMMAND_RESULT_PTR,
    COMMAND_RESULT_ERROR,
    COMMAND_RESULT_COMMAND_NOT_FOUND,
} command_result_type;

typedef struct {
    command_result_type type;
    union {
        int         i;
        float       f;
        const char *str;
        void       *ptr;
    } value;
} command_execute_result;

typedef struct command_table command_table;

// @info: opaque handle to a registered command. COMMAND_HANDLE_INVALID (0) is
// never a valid handle, so a zero-initialised handle is invalid by default.
//
// Lifetime: a handle is valid from the moment it is returned until any of the
// following occurs: the command it names is unregistered; *any other* command
// in the same table is unregistered; or the table is destroyed. Unregistering
// compacts the underlying storage and shifts the index of every later command,
// so all handles obtained before a removal must be treated as invalid
// afterwards — re-resolve with command_table_get. A handle is scoped to the
// table that produced it; passing it to a different table is undefined. The
// caller does not free handles; the table owns the storage.
typedef unsigned int command_handle;
#define COMMAND_HANDLE_INVALID 0u

void command_table_init(command_table *table);
void command_table_init_with_capacity(command_table *table,
                                      const size_t   system_command_cap,
                                      const size_t   user_defined_command_cap);
// @info: reserve capacity for at least `additional` more system commands.
void command_table_reserve_system_commands(command_table *table, const size_t additional);
// @info: reserve capacity for at least `additional` more user-defined commands.
void command_table_reserve_user_defined_commands(command_table *table,
                                                 const size_t   additional);
// @info: returns COMMAND_HANDLE_INVALID on bad arguments or duplicate name.
command_handle command_table_register(command_table *table, command_group group,
                                      const char *name,
                                      command_execute_result (*handler)(int argc,
                                                                        char **argv));
bool command_table_unregister(command_table *table, command_group group,
                              const char *name);
void command_table_destroy(command_table *table);

// @info: returns COMMAND_HANDLE_INVALID when no such command exists.
command_handle command_table_get(const command_table *table, const command_group group,
                                 const char *name);
command_execute_result command_execute_by_name(const command_table *table,
                                               const command_group  group,
                                               const char *name, int argc, char **argv);
command_execute_result command_execute(const command_table *table,
                                       const command_handle handle, int argc,
                                       char **argv);
```

- [ ] **Step 4: Add the encoder, decoder, and resolver to the implementation section**

In `ngine.lib/command.h`, immediately after `#include "collections/array_list.h"` inside the `#ifdef COMMAND_IMPLEMENTATION` block, add:

```c
#include <string.h>
```

Then, immediately after the `struct command_table { ... };` definition, add:

```c
typedef struct command command;

// @info: handle layout — ((index + 1) << 1) | group. Group occupies the low
// bit so that handle 0 stays reserved for COMMAND_HANDLE_INVALID.
static command_handle command_handle_make(const size_t index, const command_group group) {
    return (command_handle)(((index + 1u) << 1u) | (unsigned int)group);
}

static command_group command_handle_group(const command_handle handle) {
    return (command_group)(handle & 1u);
}

static size_t command_handle_index(const command_handle handle) {
    return (size_t)(handle >> 1u) - 1u;
}

static const array_list *command_table_list(const command_table *table,
                                            const command_group  group) {
    return group == COMMAND_GROUP_SYSTEM ? &table->system_commands
                                         : &table->user_defined_commands;
}

static command *command_resolve(const command_table *table, const command_handle handle) {
    if (!table || handle == COMMAND_HANDLE_INVALID) return NULL;
    return (command *)array_list_get(command_table_list(table, command_handle_group(handle)),
                                     command_handle_index(handle));
}
```

`array_list_get` already bounds-checks and returns `NULL` for an out-of-range index, which covers both an oversized handle and the wrap-around case where `handle == 1`.

- [ ] **Step 5: Convert the lookup helper to return an index**

Replace the existing `command_table_get_by_name` function in `ngine.lib/command.h` with:

```c
#define COMMAND_INDEX_NONE ((size_t)-1)

static size_t command_index_by_name(const array_list *list, const char *name) {
    for (size_t i = 0; i < list->header.size; i++) {
        command *cmd = (command *)array_list_get(list, i);
        if (strcmp(cmd->name, name) == 0) return i;
    }
    return COMMAND_INDEX_NONE;
}
```

- [ ] **Step 6: Rewrite register, get, and execute**

Replace `command_table_register`, `command_table_get`, and `command_execute` in `ngine.lib/command.h` with:

```c
command_handle command_table_register(command_table *table, command_group group,
                                      const char *name,
                                      command_execute_result (*handler)(int argc,
                                                                        char **argv)) {
    if (!table || !name || !handler) return COMMAND_HANDLE_INVALID;

    array_list *list = (array_list *)command_table_list(table, group);
    if (command_index_by_name(list, name) != COMMAND_INDEX_NONE) {
#if COMMAND_LOG_ENABLED
        log_error("command.register: command %s already exists", name);
#endif
        return COMMAND_HANDLE_INVALID;
    }

    command command = {0};
    strncpy(command.name, name, COMMAND_MAX_NAME_LEN);
    command.group   = group;
    command.handler = handler;

    const size_t index = list->header.size;
    if (!array_list_append(list, &command)) return COMMAND_HANDLE_INVALID;

#if COMMAND_LOG_ENABLED
    log_info("command.register: command %s registered", name);
#endif
    return command_handle_make(index, group);
}

command_handle command_table_get(const command_table *table, const command_group group,
                                 const char *name) {
    if (!table || !name) return COMMAND_HANDLE_INVALID;
    const size_t index = command_index_by_name(command_table_list(table, group), name);
    if (index == COMMAND_INDEX_NONE) return COMMAND_HANDLE_INVALID;
    return command_handle_make(index, group);
}

command_execute_result command_execute(const command_table *table,
                                       const command_handle handle, int argc,
                                       char **argv) {
    command *cmd = command_resolve(table, handle);
    if (!cmd) {
#if COMMAND_LOG_ENABLED
        log_error("command.execute: invalid handle %u", handle);
#endif
        return (command_execute_result){
             .type      = COMMAND_RESULT_COMMAND_NOT_FOUND,
             .value.str = "command.execute: command does not exist"  //
        };
    }
    return cmd->handler(argc, argv);
}
```

Note the original `register` guarded `group` implicitly by branching on both enum values; `command_table_list` now maps any non-`COMMAND_GROUP_SYSTEM` value to the user-defined list, which matches the enum's two-value domain.

- [ ] **Step 7: Rewrite `command_table_unregister` and `command_execute_by_name` on top of the new helper**

Replace both functions in `ngine.lib/command.h` with:

```c
bool command_table_unregister(command_table *table, command_group group,
                              const char *name) {
    if (!table || !name) return false;
    array_list  *list  = (array_list *)command_table_list(table, group);
    const size_t index = command_index_by_name(list, name);
    if (index == COMMAND_INDEX_NONE) return false;
    return array_list_remove(list, index);
}

command_execute_result command_execute_by_name(const command_table *table,
                                               const command_group  group,
                                               const char *name, int argc, char **argv) {
    const command_handle handle = command_table_get(table, group, name);
    if (handle == COMMAND_HANDLE_INVALID) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_COMMAND_NOT_FOUND,
             .value.str = "command.execute_by_name: command does not exist"  //
        };
    }
    return command_execute(table, handle, argc, argv);
}
```

- [ ] **Step 8: Port the existing tests in `ngine.lib/test_command.c`**

Every existing occurrence changes mechanically:

| Before | After |
| --- | --- |
| `command *cmd = command_table_get(&table, G, "n");` | `command_handle cmd = command_table_get(&table, G, "n");` |
| `command_table_get(...) != NULL` | `command_table_get(...) != COMMAND_HANDLE_INVALID` |
| `command_table_get(...) == NULL` | `command_table_get(...) == COMMAND_HANDLE_INVALID` |
| `command_execute(cmd, argc, argv)` | `command_execute(&table, cmd, argc, argv)` |
| `command_execute(NULL, 0, NULL)` | `command_execute(NULL, COMMAND_HANDLE_INVALID, 0, NULL)` |
| `command_table_get(NULL, G, "n") == NULL` | `command_table_get(NULL, G, "n") == COMMAND_HANDLE_INVALID` |

Lines to touch, from the current file: 184, 221, 229, 237, 244, 252, 257, 263, 274, 285, 320, 329–330, 339–340, 350–351, 361–362, 372–374, 384–386, 491, 497, 504, 511, 517, 522, 527, 534–536, 548–550. Re-grep after editing to confirm none remain:

```bash
grep -n 'command \*\|command_execute(cmd\|command_execute(missing\|command_execute(recheck' ngine.lib/test_command.c
```

Expected: no output.

Any test that asserted `command_table_register(...) == true` becomes `!= COMMAND_HANDLE_INVALID`; any that asserted `== false` becomes `== COMMAND_HANDLE_INVALID`.

- [ ] **Step 9: Run the tests to verify they pass**

```bash
make test ngine.lib/test_command.c
```

Expected: PASS, `0 failed`.

- [ ] **Step 10: Commit**

```bash
git add ngine.lib/command.h ngine.lib/test_command.c
git commit -m "refactor(command): replace command* with opaque command_handle"
```

---

### Task 2: ngine.core command propagation

**Files:**
- Modify: `ngine.core/cmd_manager.h`, `ngine.core/internal/cmd_manager/cmd_manager.c`, `ngine.core/zod_ngine.h`, `ngine.core/internal/zod_ngine/zod_ngine.c`
- Test: `ngine.core/test/test_cmd_manager.c`

**Interfaces:**
- Consumes: `command_handle`, `COMMAND_HANDLE_INVALID`, `command_table_register`, `command_table_get`, `command_execute` from Task 1.
- Produces:
  - `command_handle cmd_manager_priv_register(cmd_manager *mgr, command_group group, const char *name, command_execute_result (*handler)(int argc, char **argv));`
  - `command_handle zngine_command_register(command_group group, const char *name, command_execute_result (*handler)(int argc, char **argv));`

- [ ] **Step 1: Write the failing test**

In `ngine.core/test/test_cmd_manager.c`, replace `test_register_user_defined_command`, `test_register_null_mgr_safe`, and `test_unregister_removes_command` with:

```c
MU_TEST(test_register_user_defined_command) {
    cmd_manager mgr = {0};
    cmd_manager_priv_init(&mgr);
    command_handle h =
         cmd_manager_priv_register(&mgr, COMMAND_GROUP_USER_DEFINED, "foo", mock_handler);
    mu_check(h != COMMAND_HANDLE_INVALID);
    cmd_manager_priv_destroy(&mgr);
}

MU_TEST(test_register_null_mgr_safe) {
    mu_check(cmd_manager_priv_register(NULL, COMMAND_GROUP_SYSTEM, "foo", mock_handler) ==
             COMMAND_HANDLE_INVALID);
}

MU_TEST(test_unregister_removes_command) {
    cmd_manager mgr = {0};
    cmd_manager_priv_init(&mgr);
    cmd_manager_priv_register(&mgr, COMMAND_GROUP_USER_DEFINED, "foo", mock_handler);
    mu_check(cmd_manager_priv_unregister(&mgr, COMMAND_GROUP_USER_DEFINED, "foo") == true);
    mu_check(cmd_manager_priv_execute(&mgr, COMMAND_GROUP_USER_DEFINED, "foo", 0, NULL)
                  .type == COMMAND_RESULT_COMMAND_NOT_FOUND);
    cmd_manager_priv_destroy(&mgr);
}
```

`cmd_manager_priv_register` currently returns `bool` and `NULL` mgr is not guarded, so this both fails to compile and, once compiled, would crash — the guard is added in Step 3.

- [ ] **Step 2: Run the test to verify it fails**

```bash
make test ngine.core/test/test_cmd_manager.c
```

Expected: compilation failure — `unknown type name 'command_handle'` is already resolved by Task 1, so the failure is a type mismatch on the `cmd_manager_priv_register` return value.

- [ ] **Step 3: Update `cmd_manager`**

In `ngine.core/cmd_manager.h`, replace line 10–11 with:

```c
command_handle cmd_manager_priv_register(cmd_manager *mgr, command_group group,
                                         const char *name,
                                         command_execute_result (*handler)(int argc,
                                                                           char **argv));
```

In `ngine.core/internal/cmd_manager/cmd_manager.c`, replace `cmd_manager_priv_register` with:

```c
command_handle cmd_manager_priv_register(cmd_manager *mgr, command_group group,
                                         const char *name,
                                         command_execute_result (*handler)(int argc,
                                                                           char **argv)) {
    if (!mgr) return COMMAND_HANDLE_INVALID;
    return command_table_register(&mgr->table, group, name, handler);
}
```

Also replace `cmd_manager_priv_register_default_system_commands` — the current version asserts on `mgr->table.system_commands.header.size`, which reaches into the private table layout. Replace the whole function with:

```c
void cmd_manager_priv_register_default_system_commands(cmd_manager *mgr) {
    static const struct {
        const char *name;
        command_execute_result (*handler)(int argc, char **argv);
    } defaults[] = {
         {"reload-config-file", sys_cmd_priv_reload_config_file},
         {"show-commands", sys_cmd_priv_show_commands},
         {"set-config", sys_cmd_priv_set_config},
         {"get-config", sys_cmd_priv_get_config},
         {"list-config", sys_cmd_priv_list_config},
    };

    for (size_t i = 0; i < sizeof(defaults) / sizeof(defaults[0]); ++i) {
        const command_handle h = command_table_register(&mgr->table, COMMAND_GROUP_SYSTEM,
                                                        defaults[i].name,
                                                        defaults[i].handler);
        assert(h != COMMAND_HANDLE_INVALID && "system command registration failed");
        (void)h;
    }
}
```

Add `#include <stddef.h>` next to the existing `#include <assert.h>` in that file if `size_t` is not already available.

- [ ] **Step 4: Update `zod_ngine` command wrappers**

In `ngine.core/zod_ngine.h`, replace the `zngine_command_register` declaration with:

```c
command_handle zngine_command_register(command_group group, const char *name,
                                       command_execute_result (*handler)(int argc,
                                                                         char **argv));
```

In `ngine.core/internal/zod_ngine/zod_ngine.c`, change the definition of `zngine_command_register` to return `command_handle` and to return `cmd_manager_priv_register(...)` unchanged. Locate it with:

```bash
grep -n "zngine_command_register" ngine.core/internal/zod_ngine/zod_ngine.c
```

- [ ] **Step 5: Fix the remaining `mgr.table` reach-in in the test**

In `ngine.core/test/test_cmd_manager.c`, the other tests reference `command_table_get(&mgr.table, ...)`. Replace each with a `cmd_manager_priv_execute` assertion on the result type, matching the pattern in Step 1. Verify none remain:

```bash
grep -n "mgr\.table\|mgr->table" ngine.core/test/test_cmd_manager.c
```

Expected: no output.

- [ ] **Step 6: Run tests to verify they pass**

```bash
make test ngine.core/test
make test ngine.ext.console/test
make build-debug
```

Expected: both test runs report `0 failed`; `make build-debug` succeeds.

- [ ] **Step 7: Commit**

```bash
git add ngine.core/cmd_manager.h ngine.core/internal/cmd_manager/cmd_manager.c \
        ngine.core/zod_ngine.h ngine.core/internal/zod_ngine/zod_ngine.c \
        ngine.core/test/test_cmd_manager.c
git commit -m "refactor(cmd_manager): propagate command_handle through ngine.core"
```

---

### Task 3: event.h — subscription handles

**Files:**
- Modify: `ngine.lib/event.h`
- Test: `ngine.lib/test_event.c`

**Interfaces:**
- Consumes: nothing from earlier tasks.
- Produces:
  - `typedef unsigned int event_handle;` and `#define EVENT_HANDLE_INVALID 0u`
  - Public `event_identifier`, `event_context`, `event_callback_result` struct definitions
  - `event_handle event_table_subscribe(event_table *table, const event_category category, const int event_id, const event_callback callback, void *userdata, const event_userdata_destroy destroy_fn);`
  - `bool event_table_unsubscribe(event_table *table, const event_handle handle);`
  - Unchanged: `event_table_init`, `event_table_destroy`, `event_table_unsubscribe_by_event_identifier`, `event_table_publish`, `event_callback`, `event_userdata_destroy`.

- [ ] **Step 1: Write the failing tests**

Append to `ngine.lib/test_event.c`, before `main`:

```c
MU_TEST(test_handle_subscribe_returns_valid_handle) {
    event_table table;
    event_table_init(&table);
    event_handle h = event_table_subscribe(&table, EVENT_TAG_APPLICATION, 7, spy_callback,
                                           NULL, NULL);
    mu_check(h != EVENT_HANDLE_INVALID);
    event_table_destroy(&table);
}

MU_TEST(test_handle_subscribe_null_callback_returns_invalid) {
    event_table table;
    event_table_init(&table);
    mu_check(event_table_subscribe(&table, EVENT_TAG_APPLICATION, 7, NULL, NULL, NULL) ==
             EVENT_HANDLE_INVALID);
    event_table_destroy(&table);
}

MU_TEST(test_handle_unsubscribe_stops_delivery) {
    int          calls = 0;
    event_table  table;
    event_table_init(&table);
    event_handle h = event_table_subscribe(&table, EVENT_TAG_APPLICATION, 7, spy_callback,
                                           &calls, NULL);
    event_context ctx = {.identifier   = {.category = EVENT_TAG_APPLICATION, .event_id = 7},
                         .payload      = NULL,
                         .payload_size = 0};
    event_table_publish(&table, &ctx);
    mu_assert_int_eq(1, calls);

    mu_check(event_table_unsubscribe(&table, h) == true);
    event_table_publish(&table, &ctx);
    mu_assert_int_eq(1, calls);
    event_table_destroy(&table);
}

MU_TEST(test_handle_unsubscribe_zero_returns_false) {
    event_table table;
    event_table_init(&table);
    event_table_subscribe(&table, EVENT_TAG_APPLICATION, 7, spy_callback, NULL, NULL);
    mu_check(event_table_unsubscribe(&table, EVENT_HANDLE_INVALID) == false);
    event_table_destroy(&table);
}

MU_TEST(test_handle_unsubscribe_out_of_range_returns_false) {
    event_table table;
    event_table_init(&table);
    event_table_subscribe(&table, EVENT_TAG_APPLICATION, 7, spy_callback, NULL, NULL);
    mu_check(event_table_unsubscribe(&table, 9999u) == false);
    event_table_destroy(&table);
}

MU_TEST(test_handle_unsubscribe_twice_returns_false) {
    event_table table;
    event_table_init(&table);
    event_handle h = event_table_subscribe(&table, EVENT_TAG_APPLICATION, 7, spy_callback,
                                           NULL, NULL);
    mu_check(event_table_unsubscribe(&table, h) == true);
    mu_check(event_table_unsubscribe(&table, h) == false);
    event_table_destroy(&table);
}

MU_TEST(test_handle_subscriptions_are_distinct) {
    event_table table;
    event_table_init(&table);
    event_handle a = event_table_subscribe(&table, EVENT_TAG_APPLICATION, 7, spy_callback,
                                           NULL, NULL);
    event_handle b = event_table_subscribe(&table, EVENT_TAG_APPLICATION, 7,
                                           spy_callback_b, NULL, NULL);
    mu_check(a != b);
    mu_check(a != EVENT_HANDLE_INVALID);
    mu_check(b != EVENT_HANDLE_INVALID);
    event_table_destroy(&table);
}

MU_TEST_SUITE(event_handle_suite) {
    MU_RUN_TEST(test_handle_subscribe_returns_valid_handle);
    MU_RUN_TEST(test_handle_subscribe_null_callback_returns_invalid);
    MU_RUN_TEST(test_handle_unsubscribe_stops_delivery);
    MU_RUN_TEST(test_handle_unsubscribe_zero_returns_false);
    MU_RUN_TEST(test_handle_unsubscribe_out_of_range_returns_false);
    MU_RUN_TEST(test_handle_unsubscribe_twice_returns_false);
    MU_RUN_TEST(test_handle_subscriptions_are_distinct);
}
```

Add `MU_RUN_SUITE(event_handle_suite);` to `main`.

`spy_callback` and `spy_callback_b` already exist at lines 19 and 31 of the file. `test_handle_unsubscribe_stops_delivery` passes `&calls` as userdata; open `spy_callback` and confirm it increments through its `userdata` pointer. If it uses a file-static counter instead, adapt the test to reset and read that counter rather than passing `&calls`.

- [ ] **Step 2: Run the tests to verify they fail**

```bash
make test ngine.lib/test_event.c
```

Expected: compilation failure — `unknown type name 'event_handle'`.

- [ ] **Step 3: Rewrite the public section of `ngine.lib/event.h`**

Replace lines 1–48 of `ngine.lib/event.h` (everything before `#ifdef EVENT_IMPLEMENTATION`) with:

```c
#ifndef EVENT_H
#define EVENT_H

#include <stddef.h>
#include <stdbool.h>

typedef struct event_table event_table;

typedef enum {
    EVENT_TAG_SYSTEM_WINDOW,
    EVENT_TAG_SYSTEM_CONFIG,
    EVENT_TAG_SYSTEM_RENDERING,
    EVENT_TAG_SYSTEM_INPUT,
    EVENT_TAG_SYSTEM_AUDIO,
    EVENT_TAG_APPLICATION,
    EVENT_TAG_CUSTOM
} event_category;

typedef enum {
    EVENT_CALLBACK_RESULT_VOID,
    EVENT_CALLBACK_RESULT_INT,
    EVENT_CALLBACK_RESULT_FLOAT,
    EVENT_CALLBACK_RESULT_STRING,
    EVENT_CALLBACK_RESULT_PTR,
    EVENT_CALLBACK_RESULT_ERROR,
} event_callback_result_type;

typedef struct {
    event_category category;
    int            event_id;
} event_identifier;

typedef struct {
    event_identifier identifier;

    void  *payload;
    size_t payload_size;
} event_context;

typedef struct {
    event_callback_result_type type;
    union {
        int         i;
        float       f;
        const char *str;
        void       *ptr;
    } value;
} event_callback_result;

// userdata: caller-owned context handed back unchanged on every invocation.
// event system only stores and forwards the pointer — never allocates,
// frees, or dereferences it. Caller must outlive the subscription (or
// unsubscribe before freeing it).
typedef event_callback_result (*event_callback)(event_context *ctx, void *userdata);
// destroy_fn: optional, called exactly once when a listener is removed
// (unsubscribe or table destroy) — pass NULL to keep borrowing userdata
// (caller retains ownership, default). Non-NULL transfers ownership to the
// event system for that one subscription only.
typedef void (*event_userdata_destroy)(void *userdata);

// @info: opaque handle to one subscription. EVENT_HANDLE_INVALID (0) is never
// a valid handle, so a zero-initialised handle is invalid by default.
//
// Lifetime: a handle is valid from the moment it is returned until any of the
// following occurs: the subscription it names is removed; *any other*
// subscription in the same table is removed; or the table is destroyed.
// Removal compacts the underlying storage and shifts the index of every later
// subscription, so all handles obtained before a removal must be treated as
// invalid afterwards. A handle is scoped to the table that produced it;
// passing it to a different table is undefined. The caller does not free
// handles; the table owns the storage.
typedef unsigned int event_handle;
#define EVENT_HANDLE_INVALID 0u

void event_table_init(event_table *event_table);
void event_table_destroy(event_table *event_table);
// @info: returns EVENT_HANDLE_INVALID on bad arguments or allocation failure.
event_handle event_table_subscribe(event_table *event_table,
                                   const event_category category, const int event_id,
                                   const event_callback callback, void *userdata,
                                   const event_userdata_destroy destroy_fn);
bool event_table_unsubscribe(event_table *event_table, const event_handle handle);
bool event_table_unsubscribe_by_event_identifier(event_table         *event_table,
                                                 const event_category category,
                                                 const int            event_id);
void event_table_publish(event_table *event_table, event_context *ctx);
```

The `const` qualifiers on the members of `event_identifier`, `event_context`, and `event_callback_result` are deliberately dropped. A `const` member makes a struct non-assignable, which is a trap for a public value type; nothing in the codebase depends on those qualifiers.

- [ ] **Step 4: Delete the now-duplicated definitions from the implementation section**

In `ngine.lib/event.h`, inside the `#ifdef EVENT_IMPLEMENTATION` block, delete the `event_identifier`, `struct event_context`, and `struct event_callback_result` definitions. They are now in the public section. Keep `event_listener` and `struct event_table`. Also drop the `const` qualifiers on `event_listener`'s members so it stays assignable:

```c
typedef struct {
    event_identifier       identifier;
    event_callback         callback;
    void                  *listener_data;
    event_userdata_destroy destroy_fn;
} event_listener;
```

- [ ] **Step 5: Add the resolver**

In `ngine.lib/event.h`, immediately after `struct event_table { array_list listeners; };`, add:

```c
static event_listener *event_resolve(const event_table *table, const event_handle handle) {
    if (!table || handle == EVENT_HANDLE_INVALID) return NULL;
    return (event_listener *)array_list_get(&table->listeners, (size_t)handle - 1u);
}
```

- [ ] **Step 6: Rewrite subscribe and unsubscribe**

Replace `event_table_subscribe` and `event_table_unsubscribe` in `ngine.lib/event.h` with:

```c
event_handle event_table_subscribe(event_table *event_table,
                                   const event_category category, const int event_id,
                                   const event_callback callback, void *userdata,
                                   const event_userdata_destroy destroy_fn) {
    if (!event_table || !callback) return EVENT_HANDLE_INVALID;

    event_identifier identifier = {
         .category = category,
         .event_id = event_id  //
    };

    event_listener listener = {
         .identifier    = identifier,
         .callback      = callback,
         .listener_data = userdata,
         .destroy_fn    = destroy_fn  //
    };

    const size_t index = event_table->listeners.header.size;
    if (!array_list_append(&event_table->listeners, &listener)) {
#if EVENT_LOG_ENABLED
        log_debug("event.subscribe: append failed category=%d event_id=%d", category,
                  event_id);
#endif
        return EVENT_HANDLE_INVALID;
    }

#if EVENT_LOG_ENABLED
    log_debug("event.subscribe: category=%d event_id=%d userdata=%p handle=%u", category,
              event_id, userdata, (unsigned int)(index + 1u));
#endif
    return (event_handle)(index + 1u);
}

bool event_table_unsubscribe(event_table *event_table, const event_handle handle) {
    event_listener *l = event_resolve(event_table, handle);
    if (!l) return false;

#if EVENT_LOG_ENABLED
    log_debug("event.unsubscribe: handle=%u category=%d event_id=%d", handle,
              l->identifier.category, l->identifier.event_id);
#endif
    event_listener_release(l);
    return array_list_remove(&event_table->listeners, (size_t)handle - 1u);
}
```

- [ ] **Step 7: Port the existing tests in `ngine.lib/test_event.c`**

Any call to the old four-argument `event_table_unsubscribe(&table, category, event_id, callback, userdata)` becomes a two-argument call using the handle returned by the matching `event_table_subscribe`. Capture the handle at subscribe time:

```c
event_handle h = event_table_subscribe(&table, EVENT_TAG_APPLICATION, 7, spy_callback,
                                       &rec, NULL);
/* ... */
mu_check(event_table_unsubscribe(&table, h) == true);
```

Tests that previously unsubscribed a subscription that was never made — asserting `false` — now pass `EVENT_HANDLE_INVALID` or an unrelated out-of-range handle and still assert `false`.

Find every site:

```bash
grep -n "event_table_unsubscribe(" ngine.lib/test_event.c
```

Fix each, then re-run the grep and confirm every remaining call has exactly two arguments.

- [ ] **Step 8: Run the tests to verify they pass**

```bash
make test ngine.lib/test_event.c
```

Expected: PASS, `0 failed`.

- [ ] **Step 9: Commit**

```bash
git add ngine.lib/event.h ngine.lib/test_event.c
git commit -m "refactor(event): replace listener pointers with opaque event_handle"
```

---

### Task 4: ngine.core event propagation

**Files:**
- Modify: `ngine.core/event_manager.h`, `ngine.core/internal/event_manager/event_manager.c`, `ngine.core/zod_ngine.h`, `ngine.core/internal/zod_ngine/zod_ngine.c`
- Test: `make test ngine.core/test` plus `make build-debug`

**Interfaces:**
- Consumes: `event_handle`, `EVENT_HANDLE_INVALID`, `event_table_subscribe`, `event_table_unsubscribe` from Task 3.
- Produces:
  - `event_handle event_manager_priv_subscribe(event_manager *mgr, const event_category category, const int event_id, const event_callback callback, void *userdata, const event_userdata_destroy destroy_fn);`
  - `bool event_manager_priv_unsubscribe(event_manager *mgr, const event_handle handle);`
  - `event_handle zngine_event_subscribe(const event_category category, const int event_id, const event_callback callback, void *userdata, const event_userdata_destroy destroy_fn);`
  - `bool zngine_event_unsubscribe(const event_handle handle);`

- [ ] **Step 1: Update `event_manager.h`**

Replace lines 24–33 of `ngine.core/event_manager.h` with:

```c
event_handle event_manager_priv_subscribe(event_manager *mgr,
                                          const event_category category,
                                          const int event_id,
                                          const event_callback callback, void *userdata,
                                          const event_userdata_destroy destroy_fn);
bool event_manager_priv_unsubscribe_by_event_identifier(event_manager       *mgr,
                                                        const event_category category,
                                                        const int            event_id);
bool event_manager_priv_unsubscribe(event_manager *mgr, const event_handle handle);
```

- [ ] **Step 2: Update `event_manager.c`**

In `ngine.core/internal/event_manager/event_manager.c`, replace `event_manager_priv_subscribe` and `event_manager_priv_unsubscribe` with:

```c
event_handle event_manager_priv_subscribe(event_manager *mgr,
                                          const event_category category,
                                          const int event_id,
                                          const event_callback callback, void *userdata,
                                          const event_userdata_destroy destroy_fn) {
    if (!mgr) return EVENT_HANDLE_INVALID;
    return event_table_subscribe(&mgr->table, category, event_id, callback, userdata,
                                 destroy_fn);
}

bool event_manager_priv_unsubscribe(event_manager *mgr, const event_handle handle) {
    if (!mgr) return false;
    return event_table_unsubscribe(&mgr->table, handle);
}
```

`event_manager_priv_subscribe_sys_events` calls `event_table_subscribe` and discards the result. Leave the calls as they are; discarding an `event_handle` return is as valid as discarding the old `bool`.

- [ ] **Step 3: Update `zod_ngine.h`**

Replace the `zngine_event_subscribe` and `zngine_event_unsubscribe` declarations in `ngine.core/zod_ngine.h` with:

```c
event_handle zngine_event_subscribe(const event_category category, const int event_id,
                                    const event_callback callback, void *userdata,
                                    const event_userdata_destroy destroy_fn);
bool zngine_event_unsubscribe(const event_handle handle);
```

`zngine_event_unsubscribe_by_event_identifier` and `zngine_event_publish` are unchanged.

- [ ] **Step 4: Update `zod_ngine.c`**

Locate the two definitions:

```bash
grep -n "zngine_event_subscribe\|zngine_event_unsubscribe" ngine.core/internal/zod_ngine/zod_ngine.c
```

Change their signatures to match Step 3 and forward to `event_manager_priv_subscribe(&g_ctx.event_manager, ...)` and `event_manager_priv_unsubscribe(&g_ctx.event_manager, handle)` respectively, matching the surrounding wrapper style.

- [ ] **Step 5: Fix consumers of `zngine_event_subscribe`**

```bash
grep -rn "zngine_event_subscribe\|zngine_event_unsubscribe" ngine.example ngine.ext.console ngine.core
```

`ngine.example/engine_run_example.c:104` calls `zngine_event_subscribe` and discards the result — no change needed. Fix any site that assigns the result to a `bool` or compares it against `true`/`false`; assign to `event_handle` and compare against `EVENT_HANDLE_INVALID` instead.

- [ ] **Step 6: Run tests and builds to verify**

```bash
make test ngine.core/test
make test ngine.ext.console/test
make build-debug
make build-debug beatup
```

Expected: both test runs report `0 failed`; both builds succeed.

- [ ] **Step 7: Commit**

```bash
git add ngine.core/event_manager.h ngine.core/internal/event_manager/event_manager.c \
        ngine.core/zod_ngine.h ngine.core/internal/zod_ngine/zod_ngine.c \
        ngine.example
git commit -m "refactor(event_manager): propagate event_handle through ngine.core"
```

---

### Task 5: action.h — handles and new executor signature

**Files:**
- Modify: `ngine.lib/action.h`
- Test: `ngine.lib/test_action.c`

**Interfaces:**
- Consumes: nothing from earlier tasks.
- Produces:
  - `typedef unsigned int action_handle;` and `#define ACTION_HANDLE_INVALID 0u`
  - `typedef action_execute_result (*action_exec_fn)(const action_table *table, action_handle self, void *userdata);`
  - `typedef struct { action_exec_fn execute; } action_executor;`
  - Public `action_execute_result` struct definition
  - `action_handle action_bind(action_table *table, const action_mode context, const action_trigger_type type, const int key, const char *name, action_executor *executor);`
  - `action_handle action_resolve_by_name(const action_table *table, const action_mode context, const action_trigger_type type, const char *name);`
  - `action_handle action_resolve_by_key(const action_table *table, const action_mode context, const action_trigger_type type, const int key);`
  - `action_execute_result action_execute(const action_table *table, const action_handle handle, void *userdata);`
  - Unchanged: `action_init`, `action_destroy`, `action_rebind`, `action_unbind_by_key`, `action_unbind_by_name`, `action_unbind_all`, `action_execute_by_name`.

- [ ] **Step 1: Rewrite the test spy in `ngine.lib/test_action.c`**

Replace the `spy_record` struct and `spy_execute` function (lines 12–25) with:

```c
typedef struct {
    int                   call_count;
    action_handle         last_self;
    void                 *last_userdata;
    action_execute_result result;
} spy_record;

static action_execute_result spy_execute(const action_table *table, action_handle self,
                                         void *userdata) {
    (void)table;
    spy_record *rec = (spy_record *)userdata;
    rec->call_count++;
    rec->last_self     = self;
    rec->last_userdata = userdata;
    return rec->result;
}
```

Update `spy_record_new` and `spy_record_with_result` to initialise `.last_self = ACTION_HANDLE_INVALID` instead of `.last_action = NULL`.

- [ ] **Step 2: Write the failing handle tests**

Append to `ngine.lib/test_action.c`, before `main`:

```c
MU_TEST(test_handle_bind_returns_valid_handle) {
    action_table table;
    action_init(&table);
    action_executor exec = spy_executor_new();
    action_handle   h =
         action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    mu_check(h != ACTION_HANDLE_INVALID);
    action_destroy(&table);
}

MU_TEST(test_handle_bind_duplicate_key_returns_invalid) {
    action_table table;
    action_init(&table);
    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    mu_check(action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "duck", &exec) ==
             ACTION_HANDLE_INVALID);
    action_destroy(&table);
}

MU_TEST(test_handle_resolve_by_name_and_key_agree) {
    action_table table;
    action_init(&table);
    action_executor exec  = spy_executor_new();
    action_handle   bound = action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    mu_check(action_resolve_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "jump") == bound);
    mu_check(action_resolve_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'A') == bound);
    action_destroy(&table);
}

MU_TEST(test_handle_resolve_missing_returns_invalid) {
    action_table table;
    action_init(&table);
    mu_check(action_resolve_by_name(&table, CTX_GAME, TYPE_KEYDOWN, "nope") ==
             ACTION_HANDLE_INVALID);
    mu_check(action_resolve_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'Z') ==
             ACTION_HANDLE_INVALID);
    action_destroy(&table);
}

MU_TEST(test_handle_execute_passes_self_back) {
    action_table table;
    action_init(&table);
    action_executor exec  = spy_executor_new();
    spy_record      rec   = spy_record_new();
    action_handle   bound = action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    action_execute(&table, bound, &rec);
    mu_assert_int_eq(1, rec.call_count);
    mu_check(rec.last_self == bound);
    action_destroy(&table);
}

MU_TEST(test_handle_execute_zero_is_void) {
    action_table table;
    action_init(&table);
    action_executor exec = spy_executor_new();
    spy_record      rec  = spy_record_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    action_execute_result res = action_execute(&table, ACTION_HANDLE_INVALID, &rec);
    mu_assert_int_eq(ACTION_EXECUTE_RESULT_VOID, res.type);
    mu_assert_int_eq(0, rec.call_count);
    action_destroy(&table);
}

MU_TEST(test_handle_execute_out_of_range_is_void) {
    action_table table;
    action_init(&table);
    action_executor exec = spy_executor_new();
    spy_record      rec  = spy_record_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    action_execute_result res = action_execute(&table, 9999u, &rec);
    mu_assert_int_eq(ACTION_EXECUTE_RESULT_VOID, res.type);
    mu_assert_int_eq(0, rec.call_count);
    action_destroy(&table);
}

MU_TEST(test_handle_execute_null_table_is_void) {
    action_execute_result res = action_execute(NULL, 1u, NULL);
    mu_assert_int_eq(ACTION_EXECUTE_RESULT_VOID, res.type);
}

MU_TEST(test_handle_invalidated_by_unrelated_unbind) {
    action_table table;
    action_init(&table);
    action_executor exec = spy_executor_new();
    action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'A', "jump", &exec);
    action_handle duck_before =
         action_bind(&table, CTX_GAME, TYPE_KEYDOWN, 'B', "duck", &exec);
    action_unbind_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'A');
    action_handle duck_after = action_resolve_by_key(&table, CTX_GAME, TYPE_KEYDOWN, 'B');
    // documented behaviour: unbinding compacts storage, so earlier handles shift
    mu_check(duck_before != duck_after);
    mu_check(duck_after != ACTION_HANDLE_INVALID);
    action_destroy(&table);
}

MU_TEST_SUITE(action_handle_suite) {
    MU_RUN_TEST(test_handle_bind_returns_valid_handle);
    MU_RUN_TEST(test_handle_bind_duplicate_key_returns_invalid);
    MU_RUN_TEST(test_handle_resolve_by_name_and_key_agree);
    MU_RUN_TEST(test_handle_resolve_missing_returns_invalid);
    MU_RUN_TEST(test_handle_execute_passes_self_back);
    MU_RUN_TEST(test_handle_execute_zero_is_void);
    MU_RUN_TEST(test_handle_execute_out_of_range_is_void);
    MU_RUN_TEST(test_handle_execute_null_table_is_void);
    MU_RUN_TEST(test_handle_invalidated_by_unrelated_unbind);
}
```

Add `MU_RUN_SUITE(action_handle_suite);` to `main`.

- [ ] **Step 3: Run the tests to verify they fail**

```bash
make test ngine.lib/test_action.c
```

Expected: compilation failure — `unknown type name 'action_handle'`.

- [ ] **Step 4: Rewrite the public section of `ngine.lib/action.h`**

Replace lines 1–49 of `ngine.lib/action.h` (everything before `#ifdef ACTION_IMPLEMENTATION`) with:

```c
#ifndef ACTION_H
#define ACTION_H

#include <stddef.h>
#include <stdbool.h>

typedef struct action_table action_table;
typedef int                 action_trigger_type;
typedef int                 action_mode;

typedef enum {
    ACTION_EXECUTE_RESULT_VOID,
    ACTION_EXECUTE_RESULT_INT,
    ACTION_EXECUTE_RESULT_FLOAT,
    ACTION_EXECUTE_RESULT_STRING,
    ACTION_EXECUTE_RESULT_PTR,
    ACTION_EXECUTE_RESULT_ERROR,
} action_execute_result_type;

typedef struct {
    action_execute_result_type type;
    union {
        int         i;
        float       f;
        const char *str;
        void       *ptr;
    } value;
} action_execute_result;

// @info: opaque handle to a bound action. ACTION_HANDLE_INVALID (0) is never a
// valid handle, so a zero-initialised handle is invalid by default.
//
// Lifetime: a handle is valid from the moment it is returned until any of the
// following occurs: the action it names is unbound; *any other* action in the
// same table is unbound; or the table is destroyed. Unbinding compacts the
// underlying storage and shifts the index of every later action, so all
// handles obtained before an unbind must be treated as invalid afterwards —
// re-resolve with action_resolve_by_name or action_resolve_by_key. A handle is
// scoped to the table that produced it; passing it to a different table is
// undefined. The caller does not free handles; the table owns the storage.
typedef unsigned int action_handle;
#define ACTION_HANDLE_INVALID 0u

// @info: `self` is the handle of the action being executed. It is an identity
// token — compare it against handles you already hold, or pass it back to
// action_execute or an unbind function. There is no way to read the action's
// name, key, context, or trigger type back out of it.
typedef action_execute_result (*action_exec_fn)(const action_table *table,
                                                action_handle self, void *userdata);

typedef struct {
    action_exec_fn execute;
} action_executor;

void action_init(action_table *table);
void action_destroy(action_table *table);

// @info: both return ACTION_HANDLE_INVALID when no such action is bound.
action_handle action_resolve_by_name(const action_table *table, const action_mode context,
                                     const action_trigger_type type, const char *name);
action_handle action_resolve_by_key(const action_table *table, const action_mode context,
                                    const action_trigger_type type, const int key);

// @info: returns ACTION_HANDLE_INVALID on bad arguments, or when the
// (context, type, key) triple is already bound.
action_handle action_bind(action_table *table, const action_mode context,
                          const action_trigger_type type, const int key,
                          const char *name, action_executor *executor);

bool action_rebind(action_table *table, const action_mode context,
                   const action_trigger_type type, const int old_key, const int new_key);

bool action_unbind_by_key(action_table *table, const action_mode context,
                          const action_trigger_type type, const int key);

bool action_unbind_by_name(action_table *table, const action_mode context,
                           const action_trigger_type type, const char *name);

bool action_unbind_all(action_table *table, const action_mode context,
                       const action_trigger_type type);

action_execute_result action_execute(const action_table *table,
                                     const action_handle handle, void *userdata);
action_execute_result action_execute_by_name(action_table             *table,
                                             const action_mode         context,
                                             const action_trigger_type type,
                                             const char *name, void *userdata);
```

- [ ] **Step 5: Update the implementation section's private types**

In `ngine.lib/action.h`, inside `#ifdef ACTION_IMPLEMENTATION`, delete the `struct action_execute_result` and `struct action_executor` definitions — both are now public. Keep and adjust the rest:

```c
typedef struct action action;

typedef struct {
    char name[ACTION_NAME_MAX];
    int  key;
} action_binding;

struct action {
    action_executor     executor;
    action_binding      binding;
    action_mode         context;
    action_trigger_type type;
};

struct action_table {
    array_list actions;
};
```

- [ ] **Step 6: Add the index helpers and resolver**

Immediately after `struct action_table { array_list actions; };` in `ngine.lib/action.h`, add:

```c
#define ACTION_INDEX_NONE ((size_t)-1)

static action_handle action_handle_make(const size_t index) {
    return (action_handle)(index + 1u);
}

static action *action_resolve(const action_table *table, const action_handle handle) {
    if (!table || handle == ACTION_HANDLE_INVALID) return NULL;
    return (action *)array_list_get(&table->actions, (size_t)handle - 1u);
}

static size_t action_index_by_name(const action_table *table, const action_mode context,
                                   const action_trigger_type type, const char *name) {
    if (!table || !name) return ACTION_INDEX_NONE;
    for (size_t i = 0; i < table->actions.header.size; i++) {
        action *entry = (action *)array_list_get(&table->actions, i);
        if (entry->context == context && entry->type == type &&
            strcmp(entry->binding.name, name) == 0) {
            return i;
        }
    }
    return ACTION_INDEX_NONE;
}

static size_t action_index_by_key(const action_table *table, const action_mode context,
                                  const action_trigger_type type, const int key) {
    if (!table) return ACTION_INDEX_NONE;
    for (size_t i = 0; i < table->actions.header.size; i++) {
        action *entry = (action *)array_list_get(&table->actions, i);
        if (entry->context == context && entry->type == type &&
            entry->binding.key == key) {
            return i;
        }
    }
    return ACTION_INDEX_NONE;
}
```

- [ ] **Step 7: Rewrite resolve, bind, unbind, rebind, and execute on top of the helpers**

Replace `action_resolve_by_name`, `action_resolve_by_key`, `action_bind`, `action_rebind`, `action_unbind_by_key`, `action_unbind_by_name`, `action_execute`, and `action_execute_by_name` in `ngine.lib/action.h` with:

```c
action_handle action_resolve_by_name(const action_table *table, const action_mode context,
                                     const action_trigger_type type, const char *name) {
    const size_t index = action_index_by_name(table, context, type, name);
    if (index == ACTION_INDEX_NONE) return ACTION_HANDLE_INVALID;
    return action_handle_make(index);
}

action_handle action_resolve_by_key(const action_table *table, const action_mode context,
                                    const action_trigger_type type, const int key) {
    const size_t index = action_index_by_key(table, context, type, key);
    if (index == ACTION_INDEX_NONE) return ACTION_HANDLE_INVALID;
    return action_handle_make(index);
}

action_handle action_bind(action_table *table, const action_mode context,
                          const action_trigger_type type, const int key,
                          const char *name, action_executor *executor) {
    if (!table || !executor || !name || strlen(name) >= ACTION_NAME_MAX)
        return ACTION_HANDLE_INVALID;

    if (action_index_by_key(table, context, type, key) != ACTION_INDEX_NONE)
        return ACTION_HANDLE_INVALID;

    action new_action = {.context  = context,
                         .type     = type,
                         .binding  = {.key = key},
                         .executor = *executor};
    memcpy(new_action.binding.name, name, strlen(name) + 1);

    const size_t index = table->actions.header.size;
    if (!array_list_append(&table->actions, &new_action)) return ACTION_HANDLE_INVALID;
    return action_handle_make(index);
}

bool action_rebind(action_table *table, const action_mode context,
                   const action_trigger_type type, const int old_key, const int new_key) {
    const size_t index = action_index_by_key(table, context, type, old_key);
    if (index == ACTION_INDEX_NONE) return false;
    action *entry      = (action *)array_list_get(&table->actions, index);
    entry->binding.key = new_key;
    return true;
}

bool action_unbind_by_key(action_table *table, const action_mode context,
                          const action_trigger_type type, const int key) {
    const size_t index = action_index_by_key(table, context, type, key);
    if (index == ACTION_INDEX_NONE) return false;
    return array_list_remove(&table->actions, index);
}

bool action_unbind_by_name(action_table *table, const action_mode context,
                           const action_trigger_type type, const char *name) {
    const size_t index = action_index_by_name(table, context, type, name);
    if (index == ACTION_INDEX_NONE) return false;
    return array_list_remove(&table->actions, index);
}

action_execute_result action_execute(const action_table *table,
                                     const action_handle handle, void *userdata) {
    action *entry = action_resolve(table, handle);
    if (!entry) return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
    return entry->executor.execute(table, handle, userdata);
}

action_execute_result action_execute_by_name(action_table             *table,
                                             const action_mode         context,
                                             const action_trigger_type type,
                                             const char *name, void *userdata) {
    const action_handle handle = action_resolve_by_name(table, context, type, name);
    if (handle == ACTION_HANDLE_INVALID)
        return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
    return action_execute(table, handle, userdata);
}
```

`action_unbind_all` is unchanged — it already iterates and removes by index without exposing pointers.

- [ ] **Step 8: Port the existing tests in `ngine.lib/test_action.c`**

Mechanical changes:

| Before | After |
| --- | --- |
| `action *bound = action_resolve_by_key(...)` | `action_handle bound = action_resolve_by_key(...)` |
| `action_resolve_by_name(...) != NULL` | `action_resolve_by_name(...) != ACTION_HANDLE_INVALID` |
| `action_resolve_by_name(...) == NULL` | `action_resolve_by_name(...) == ACTION_HANDLE_INVALID` |
| `action_execute(bound, &rec)` | `action_execute(&table, bound, &rec)` |
| `action_execute(NULL, NULL)` | `action_execute(NULL, ACTION_HANDLE_INVALID, NULL)` |
| `action_bind(...)` asserted `== true` | `!= ACTION_HANDLE_INVALID` |
| `action_bind(...)` asserted `== false` | `== ACTION_HANDLE_INVALID` |
| `rec.last_action` | `rec.last_self` |

Lines to touch, from the current file: 132, 144, 150, 160, 172, 178, 189–190, 240, 285, 320–322, 341, 350–354, 367–373, 386–389, 401–402, 409–410, 420–431, 443–457, 463–464. Test at line 447 (`mu_check(bound == action_resolve_by_key(...))`) works unchanged once both sides are `action_handle`. Test at line 455 (`action_resolve_by_key(...) == bound`) is a **rebind**, not a removal, so the handle stays valid and the assertion still holds.

Verify none remain:

```bash
grep -n 'action \*\|last_action\|action_execute(bound\|action_execute(NULL, NULL)' ngine.lib/test_action.c
```

Expected: no output.

- [ ] **Step 9: Run the tests to verify they pass**

```bash
make test ngine.lib/test_action.c
make test ngine.lib
```

Expected: both report `0 failed`.

- [ ] **Step 10: Commit**

```bash
git add ngine.lib/action.h ngine.lib/test_action.c
git commit -m "refactor(action): replace action* with opaque action_handle"
```

---

### Task 6: ngine.core action propagation and example port

**Files:**
- Modify: `ngine.core/action_manager.h`, `ngine.core/internal/action_manager/action_manager.c`, `ngine.core/zod_ngine.h`, `ngine.core/internal/zod_ngine/zod_ngine_action.c`, `ngine.example/engine_run_example.c`, `ngine.example/beatup.c`
- Test: `make test`, `make build-debug`, `make build-debug beatup`

**Interfaces:**
- Consumes: `action_handle`, `ACTION_HANDLE_INVALID`, `action_exec_fn`, `action_executor`, `action_bind`, `action_resolve_by_name`, `action_resolve_by_key`, `action_execute` from Task 5.
- Produces:
  - `action_handle action_manager_priv_resolve_by_name(const action_manager *mgr, const action_mode context, const action_trigger_type type, const char *name);`
  - `action_handle action_manager_priv_resolve_by_key(const action_manager *mgr, const action_mode context, const action_trigger_type type, const int key);`
  - `action_handle action_manager_priv_bind(action_manager *mgr, const action_mode context, const action_trigger_type type, const int key, const char *name, action_executor *executor);`
  - `action_execute_result action_manager_priv_execute(action_manager *mgr, const action_handle handle, void *userdata);`
  - `action_handle zngine_action_resolve_by_name(const action_mode context, const action_trigger_type type, const char *name);`
  - `action_handle zngine_action_resolve_by_key(const action_mode context, const action_trigger_type type, const int key);`
  - `action_handle zngine_action_bind(const action_mode context, const action_trigger_type type, const int key, const char *name, action_executor *executor);`
  - `action_execute_result zngine_action_execute(const action_handle handle, void *userdata);`

- [ ] **Step 1: Update `action_manager.h`**

Replace lines 10–20 and line 33 of `ngine.core/action_manager.h` with:

```c
action_handle action_manager_priv_resolve_by_name(const action_manager     *mgr,
                                                  const action_mode         context,
                                                  const action_trigger_type type,
                                                  const char               *name);
action_handle action_manager_priv_resolve_by_key(const action_manager     *mgr,
                                                 const action_mode         context,
                                                 const action_trigger_type type,
                                                 const int                 key);

action_handle action_manager_priv_bind(action_manager *mgr, const action_mode context,
                                       const action_trigger_type type, const int key,
                                       const char *name, action_executor *executor);
```

and

```c
action_execute_result action_manager_priv_execute(action_manager     *mgr,
                                                  const action_handle handle,
                                                  void               *userdata);
```

`action_manager_priv_execute` now needs the manager, because the handle alone cannot reach the table.

- [ ] **Step 2: Update `action_manager.c`**

In `ngine.core/internal/action_manager/action_manager.c`, change the four affected function definitions to match Step 1 and add `NULL` guards on `mgr`:

```c
action_handle action_manager_priv_resolve_by_name(const action_manager     *mgr,
                                                  const action_mode         context,
                                                  const action_trigger_type type,
                                                  const char               *name) {
    if (!mgr) return ACTION_HANDLE_INVALID;
    return action_resolve_by_name(&mgr->table, context, type, name);
}

action_handle action_manager_priv_resolve_by_key(const action_manager     *mgr,
                                                 const action_mode         context,
                                                 const action_trigger_type type,
                                                 const int                 key) {
    if (!mgr) return ACTION_HANDLE_INVALID;
    return action_resolve_by_key(&mgr->table, context, type, key);
}

action_handle action_manager_priv_bind(action_manager *mgr, const action_mode context,
                                       const action_trigger_type type, const int key,
                                       const char *name, action_executor *executor) {
    if (!mgr) return ACTION_HANDLE_INVALID;
    return action_bind(&mgr->table, context, type, key, name, executor);
}

action_execute_result action_manager_priv_execute(action_manager     *mgr,
                                                  const action_handle handle,
                                                  void               *userdata) {
    if (!mgr) return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
    return action_execute(&mgr->table, handle, userdata);
}
```

The remaining functions in that file — rebind, the three unbind variants, execute_by_name, init, destroy — are unchanged.

- [ ] **Step 3: Update `zod_ngine.h`**

Replace the action accessor block in `ngine.core/zod_ngine.h` with:

```c
action_handle zngine_action_resolve_by_name(const action_mode         context,
                                            const action_trigger_type type,
                                            const char               *name);
action_handle zngine_action_resolve_by_key(const action_mode         context,
                                           const action_trigger_type type,
                                           const int                 key);

action_handle zngine_action_bind(const action_mode context,
                                 const action_trigger_type type, const int key,
                                 const char *name, action_executor *executor);
bool zngine_action_rebind(const action_mode context, const action_trigger_type type,
                          const int old_key, const int new_key);

bool zngine_action_unbind_by_key(const action_mode context, const action_trigger_type type,
                                 const int key);
bool zngine_action_unbind_by_name(const action_mode context,
                                  const action_trigger_type type, const char *name);
bool zngine_action_unbind_all(const action_mode context, const action_trigger_type type);

action_execute_result zngine_action_execute(const action_handle handle, void *userdata);
action_execute_result zngine_action_execute_by_name(const action_mode         context,
                                                    const action_trigger_type type,
                                                    const char *name, void *userdata);
```

- [ ] **Step 4: Update `zod_ngine_action.c`**

In `ngine.core/internal/zod_ngine/zod_ngine_action.c`, change the four affected definitions:

```c
action_handle zngine_action_resolve_by_name(const action_mode         context,
                                            const action_trigger_type type,
                                            const char               *name) {
    return action_manager_priv_resolve_by_name(&g_ctx.action_manager, context, type,
                                               name);
}

action_handle zngine_action_resolve_by_key(const action_mode         context,
                                           const action_trigger_type type,
                                           const int                 key) {
    return action_manager_priv_resolve_by_key(&g_ctx.action_manager, context, type, key);
}

action_handle zngine_action_bind(const action_mode context,
                                 const action_trigger_type type, const int key,
                                 const char *name, action_executor *executor) {
    return action_manager_priv_bind(&g_ctx.action_manager, context, type, key, name,
                                    executor);
}

action_execute_result zngine_action_execute(const action_handle handle, void *userdata) {
    return action_manager_priv_execute(&g_ctx.action_manager, handle, userdata);
}
```

The other functions in that file are unchanged.

- [ ] **Step 5: Port `ngine.example/engine_run_example.c`**

Replace `toggle_console_execute` (lines 16–23) with:

```c
static action_execute_result toggle_console_execute(const action_table *table,
                                                    action_handle       self,
                                                    void               *userdata) {
    (void)table;
    (void)self;
    (void)userdata;
    zconsole_toggle();
    return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
}
```

The `zngine_action_bind` call at line 109 discards its return value, which is still valid. The `zngine_action_execute_by_name` call at line 140 is unchanged.

- [ ] **Step 6: Port `ngine.example/beatup.c`**

```bash
grep -n "action_execute_result\|action_executor\|zngine_action_\|const action \*" ngine.example/beatup.c
```

Apply the same executor-signature change to any executor callback found. If the grep returns only the `event_context` construction at line 260, no change is needed in this file.

- [ ] **Step 7: Verify the whole tree builds and passes**

```bash
make test
make build-debug
make build-debug beatup
```

Expected: `make test` reports `0 failed` across all four test directories; both builds succeed.

- [ ] **Step 8: Commit**

```bash
git add ngine.core/action_manager.h ngine.core/internal/action_manager/action_manager.c \
        ngine.core/zod_ngine.h ngine.core/internal/zod_ngine/zod_ngine_action.c \
        ngine.example/engine_run_example.c ngine.example/beatup.c
git commit -m "refactor(action_manager): propagate action_handle through ngine.core"
```

---

### Task 7: Boundary check and final verification

**Files:**
- Create: `tools/check_public_boundary.sh`
- Test: `make test-asan`, `make build-debug`

**Interfaces:**
- Consumes: the finished headers from Tasks 1, 3, and 5.
- Produces: `tools/check_public_boundary.sh`, exiting `0` when the public sections are clean and `1` otherwise.

- [ ] **Step 1: Write the boundary check script**

Create `tools/check_public_boundary.sh`:

```sh
#!/bin/sh
# Fails if the public section of a module header (everything before its
# #ifdef *_IMPLEMENTATION guard) mentions a private record type.
set -u

status=0

check() {
    header="$1"
    guard="$2"
    shift 2

    public=$(awk -v guard="#ifdef $guard" '$0 ~ guard { exit } { print }' "$header")

    for pattern in "$@"; do
        if printf '%s\n' "$public" | grep -qE "$pattern"; then
            echo "check-public-boundary: $header leaks: $pattern"
            printf '%s\n' "$public" | grep -nE "$pattern" | sed 's/^/    /'
            status=1
        fi
    done
}

check ngine.lib/command.h COMMAND_IMPLEMENTATION \
    'struct[[:space:]]+command[[:space:]]*\{' \
    '(^|[^_[:alnum:]])command[[:space:]]*\*'

check ngine.lib/action.h ACTION_IMPLEMENTATION \
    'struct[[:space:]]+action[[:space:]]*\{' \
    'struct[[:space:]]+action_binding[[:space:]]*\{' \
    '(^|[^_[:alnum:]])action[[:space:]]*\*' \
    '(^|[^_[:alnum:]])action_binding[[:space:]]*\*'

check ngine.lib/event.h EVENT_IMPLEMENTATION \
    'event_listener'

if [ "$status" -eq 0 ]; then
    echo "check-public-boundary: OK"
fi

exit "$status"
```

Make it executable:

```bash
chmod +x tools/check_public_boundary.sh
```

- [ ] **Step 2: Run the script to verify it passes**

```bash
./tools/check_public_boundary.sh
```

Expected: `check-public-boundary: OK`, exit status `0`.

- [ ] **Step 3: Prove the script actually detects a leak**

Temporarily add `command *command_table_peek(const command_table *table);` to the public section of `ngine.lib/command.h`, then run:

```bash
./tools/check_public_boundary.sh; echo "exit=$?"
```

Expected: a `leaks` line naming `ngine.lib/command.h`, and `exit=1`.

Remove the temporary line, re-run, and confirm `check-public-boundary: OK` and `exit=0`.

- [ ] **Step 4: Confirm no pointer types remain anywhere in the public surface**

```bash
grep -rn 'action \*\|command \*\|event_listener' \
    ngine.lib/command.h ngine.lib/event.h ngine.lib/action.h \
    ngine.core/action_manager.h ngine.core/event_manager.h \
    ngine.core/cmd_manager.h ngine.core/zod_ngine.h
```

Expected: matches only inside the `#ifdef *_IMPLEMENTATION` sections of the three `ngine.lib` headers, and nothing at all in the four `ngine.core` headers.

- [ ] **Step 5: Full suite, including ASan**

```bash
make test
make test-asan
make build-debug
make build-debug beatup
```

Expected: both test runs report `0 failed`; no ASan leak or use-after-free report; both builds succeed.

- [ ] **Step 6: Smoke-run the engine**

```bash
timeout 5 ./engine_run --log-level trace; echo "exit=$?"
```

Expected: the window opens and the process is killed by the timeout (`exit=124`). A non-zero exit other than `124`, or a crash before the timeout, means an action or event handle regressed.

- [ ] **Step 7: Commit**

```bash
git add tools/check_public_boundary.sh
git commit -m "test: add public-boundary check for opaque handle headers"
```

---

## Self-Review

**Spec coverage:**

| Spec section | Task |
| --- | --- |
| Handle representation (`unsigned int`, `0` invalid, group packing) | 1, 3, 5 |
| Handle resolution (private static resolvers) | 1 step 4, 3 step 5, 5 step 6 |
| Public value types promoted | 3 step 3, 5 step 4 |
| Public API: command | 1 |
| Public API: event | 3 |
| Public API: action | 5 |
| Error contract | 1 steps 6–7, 3 step 6, 5 step 7, guards added in 2, 4, 6 |
| No attribute accessors | Global Constraints; no task adds getters |
| Handle lifetime documented in headers | 1 step 3, 3 step 3, 5 step 4 |
| Propagation through ngine.core | 2, 4, 6 |
| Files affected | 1–7 cover every listed path |
| Testing: round trip | 1 step 1, 3 step 1, 5 step 2 |
| Testing: invalid handle | 1 step 1, 3 step 1, 5 step 2 |
| Testing: handle 0 default | 1 step 1, 3 step 1, 5 step 2 |
| Testing: documented invalidation | 1 step 1, 5 step 2 |
| Testing: group encoding | 1 step 1 |
| Testing: boundary check | 7 |

No gaps.

**Type consistency:** `command_handle`, `event_handle`, `action_handle`, `COMMAND_HANDLE_INVALID`, `EVENT_HANDLE_INVALID`, `ACTION_HANDLE_INVALID`, `action_exec_fn`, `command_handle_make`, `command_handle_group`, `command_handle_index`, `command_table_list`, `command_resolve`, `command_index_by_name`, `COMMAND_INDEX_NONE`, `event_resolve`, `action_handle_make`, `action_resolve`, `action_index_by_name`, `action_index_by_key`, `ACTION_INDEX_NONE` are each defined once and spelled identically at every later use. `action_manager_priv_execute` takes `(action_manager *, action_handle, void *)` in Task 6 steps 1 and 2 and is called that way in step 4.
