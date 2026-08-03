# Opaque Handles for Command, Event, and Action

**Date:** 2026-08-03
**Status:** Approved, ready for implementation planning

## Problem

The `command`, `event`, and `action` modules in `ngine.lib` leak implementation
pointers across their public boundary. `command_table_get` returns `command *`,
`action_resolve_by_name` and `action_resolve_by_key` return `action *`, and the
`action_executor.execute` callback receives a `const action *`. Those pointers
propagate upward through `ngine.core` (`action_manager.h`, `zod_ngine.h`) and
reach application code in `ngine.example`.

Callers therefore hold raw pointers into `array_list` storage. Any append that
triggers a reallocation, or any removal that compacts the list, silently
invalidates every outstanding pointer. Nothing in the type system marks that
hazard, and nothing at runtime detects it.

A second, independent defect exists today: several value types
(`action_execute_result`, `action_executor`, `event_callback_result`,
`event_context`, `event_identifier`) are forward-declared in the public section
but defined only inside `#ifdef *_IMPLEMENTATION`. Public functions return or
accept them by value. A consumer that includes the header without defining the
implementation macro cannot compile against those signatures. This refactor
fixes that as part of drawing the public boundary correctly.

## Goal

Replace every public-facing `command *`, `event`-related pointer, and `action *`
with an opaque integer handle. After the refactor, no public header exposes the
layout of `struct command`, `struct action`, or the event listener record, and
no public function signature mentions those types.

## Non-goals

- Changing the ownership model of the tables. `command_table`, `event_table`,
  and `action_table` remain caller-allocated, already-opaque structs, and their
  init/destroy lifecycle is unchanged.
- Changing storage from `array_list` to a slot allocator. This was considered
  and explicitly declined in favour of the simpler index-based handle; see
  "Rejected alternative" below.
- Any refactoring of modules unrelated to command, event, and action.

## Handle representation

```c
typedef unsigned int command_handle;
typedef unsigned int event_handle;
typedef unsigned int action_handle;

#define COMMAND_HANDLE_INVALID 0u
#define EVENT_HANDLE_INVALID   0u
#define ACTION_HANDLE_INVALID  0u
```

Zero is reserved as the invalid handle for all three types, so a zero-initialised
struct field or a `{0}` designated initialiser yields an invalid handle by
default.

`event` and `action` each store their records in a single `array_list`, so their
encoding is:

```
handle = index + 1
index  = handle - 1
```

`command` stores records in two lists, `system_commands` and
`user_defined_commands`. Rather than force callers to carry a `command_group`
alongside every handle, the group is packed into the low bit:

```
handle = ((index + 1) << 1) | (unsigned int)group
group  = (command_group)(handle & 1u)
index  = (handle >> 1) - 1
```

`COMMAND_GROUP_SYSTEM` is 0 and `COMMAND_GROUP_USER_DEFINED` is 1, matching the
existing enum. Handle 0 remains invalid under this encoding because a valid
handle always has a non-zero `index + 1` in the upper bits.

Handles are values. The caller never frees a handle; the table owns the
underlying storage.

## Handle resolution

Each module gains one private resolver, declared `static` inside its
`#ifdef *_IMPLEMENTATION` block:

```c
static command *command_resolve(const command_table *table, command_handle handle);
static action  *action_resolve (const action_table  *table, action_handle  handle);
static event_listener *event_resolve(const event_table *table, event_handle handle);
```

Each returns `NULL` when the handle is `0`, when the decoded index is at or
beyond the list size, or when the table pointer is `NULL`. Because the handle
type is unsigned, the guard is `handle == 0` rather than a sign check.

Resolvers are never exposed. Public functions that need to reach a record call
the resolver internally and translate a `NULL` result into the module's
documented error value.

## Public value types

The following types move from the implementation section to the public section
of their headers, because public signatures use them by value:

- `command_execute_result` (already public; unchanged)
- `action_execute_result`
- `action_executor`
- `event_callback_result`
- `event_context`
- `event_identifier`

The following types stay private, defined only under `*_IMPLEMENTATION`:

- `struct command`
- `struct command_table`
- `struct action`
- `struct action_binding`
- `struct action_table`
- `struct event_table`
- `event_listener`

The distinction is identity versus value. A type that names a live, table-owned
entity stays hidden behind a handle. A plain data carrier that crosses the
boundary by value becomes public.

## Public API: command

```c
command_handle command_table_register(command_table *table,
                                      command_group  group,
                                      const char    *name,
                                      command_execute_result (*handler)(int argc,
                                                                        char **argv));

command_handle command_table_get(const command_table *table,
                                 command_group        group,
                                 const char          *name);

bool command_table_unregister(command_table *table,
                              command_group  group,
                              const char    *name);

command_execute_result command_execute(const command_table *table,
                                       command_handle       handle,
                                       int argc, char **argv);

command_execute_result command_execute_by_name(const command_table *table,
                                               command_group        group,
                                               const char *name,
                                               int argc, char **argv);
```

`command_table_register` changes from returning `bool` to returning a handle;
`COMMAND_HANDLE_INVALID` signals failure, which covers every case the old
`false` covered. `command_table_init`, `command_table_init_with_capacity`,
`command_table_destroy`, and both reserve functions are unchanged.

## Public API: event

`event_handle` identifies a **subscription**, not an event payload. A
subscription is the thing with a lifetime; `event_context` remains a
caller-constructed value type used for publishing and for callback delivery.

```c
event_handle event_table_subscribe(event_table           *table,
                                   event_category         category,
                                   int                    event_id,
                                   event_callback         callback,
                                   void                  *userdata,
                                   event_userdata_destroy destroy_fn);

bool event_table_unsubscribe(event_table *table, event_handle handle);

bool event_table_unsubscribe_by_event_identifier(event_table   *table,
                                                 event_category category,
                                                 int            event_id);

void event_table_publish(event_table *table, event_context *ctx);
```

`event_table_subscribe` changes from returning `bool` to returning a handle. The
previous four-argument `event_table_unsubscribe(table, category, event_id,
callback, userdata)` is removed and replaced by the handle form.
`event_table_unsubscribe_by_event_identifier` is retained unchanged; it matches
on identifier rather than on a pointer, so it was never part of the leak. The
`event_callback` typedef and `event_table_publish` are unchanged.

## Public API: action

```c
typedef action_execute_result (*action_exec_fn)(const action_table *table,
                                                action_handle       self,
                                                void               *userdata);

struct action_executor {
    action_exec_fn execute;
};

action_handle action_bind(action_table       *table,
                          action_mode         context,
                          action_trigger_type type,
                          int                 key,
                          const char         *name,
                          action_executor    *executor);

action_handle action_resolve_by_name(const action_table *table,
                                     action_mode         context,
                                     action_trigger_type type,
                                     const char         *name);

action_handle action_resolve_by_key(const action_table *table,
                                    action_mode         context,
                                    action_trigger_type type,
                                    int                 key);

action_execute_result action_execute(const action_table *table,
                                     action_handle       handle,
                                     void               *userdata);

action_execute_result action_execute_by_name(action_table       *table,
                                             action_mode         context,
                                             action_trigger_type type,
                                             const char *name, void *userdata);
```

The executor callback changes from `(const action *action, void *userdata)` to
`(const action_table *table, action_handle self, void *userdata)`. `self` is an
identity token, not an accessor into the record: a callback can compare it
against handles it already holds, or pass it back to `action_execute` or an
unbind function. The table pointer is required because handles are scoped to a
single table.

`action_bind` changes from returning `bool` to returning a handle.
`action_rebind`, `action_unbind_by_key`, `action_unbind_by_name`,
`action_unbind_all`, `action_init`, and `action_destroy` are unchanged; they
already operate on keys, names, and the table.

## Error contract

The contract is identical across all three modules.

| Situation | Result |
| --- | --- |
| Lookup miss, or registration/binding failure | `*_HANDLE_INVALID` (`0u`) |
| `*_execute` with an invalid handle | The module's existing not-found or VOID result; no crash |

No assertions and no aborts. This matches the existing NULL-tolerant style of
all three modules, where every public entry point already guards against a
`NULL` table.

### No attribute accessors

The public API deliberately provides **no** getters for record fields — no
`command_get_name`, no `action_get_key`, and no equivalents. A handle is an
identity token and nothing more.

The consequence is that a handle is inert once obtained: it can be executed,
unbound, or compared for equality against another handle, but its underlying
name, key, context, group, and trigger type are not readable back through the
public boundary. Callers that need those values must retain them from the call
site that supplied them in the first place — `action_bind` and
`command_table_register` are both given the name and key by the caller.

This keeps the boundary minimal. If a genuine need for read-back appears later,
accessors can be added without changing anything defined here.

## Handle lifetime

The following paragraph goes into each of the three public headers, adjusted for
the module name.

> A handle is valid from the moment it is returned until any of the following
> occurs: the entry it names is removed; **any other entry in the same table is
> removed**; or the table is destroyed. Removal compacts the underlying storage,
> which shifts the index of every entry after the removed one, so all handles
> obtained before a removal must be treated as invalid afterwards. Re-resolve by
> name or key after any removal. A handle is scoped to the table that produced
> it; passing a handle to a different table is undefined behaviour. The caller
> does not free handles — the table owns the storage.

### Known hazard, accepted

Because handles are plain indices, a stale handle used after an unrelated
removal does not resolve to `NULL`. It resolves to a *different, live* entry.
`event_table_unsubscribe(table, stale_handle)` can therefore remove the wrong
listener, and `action_execute(table, stale_handle, ud)` can run the wrong
action. This is a silent wrong-object hazard, not merely a stale read.

This was raised during design and the plain-index encoding was chosen anyway,
for its minimal diff and zero storage change. The mitigation is the documented
lifetime rule above: re-resolve after any removal.

### Rejected alternative

A slot array with tombstones, a free list, and a generation counter packed into
the handle (`handle = (generation << 16) | slot`) would detect stale handles and
resolve them to `NULL`. It was rejected for this iteration because it requires
replacing `array_list` storage in all three tables. If the wrong-object hazard
proves to bite in practice, this is the upgrade path, and it is source-compatible
with the public API defined here — only the private encoding and resolvers
change.

## Propagation through ngine.core

`zod_ngine` owns the global engine context and can resolve the relevant table
internally, so its public functions take only a handle:

```c
action_handle zngine_action_resolve_by_name(action_mode context,
                                            action_trigger_type type,
                                            const char *name);
action_handle zngine_action_resolve_by_key (action_mode context,
                                            action_trigger_type type,
                                            int key);
action_handle zngine_action_bind(action_mode context, action_trigger_type type,
                                 int key, const char *name,
                                 action_executor *executor);
action_execute_result zngine_action_execute(action_handle handle, void *userdata);
```

`action_manager.h` sits below `zod_ngine` and still takes an explicit manager:

```c
action_handle action_manager_priv_resolve_by_name(const action_manager *mgr, ...);
action_handle action_manager_priv_resolve_by_key (const action_manager *mgr, ...);
action_execute_result action_manager_priv_execute(action_manager *mgr,
                                                  action_handle   handle,
                                                  void           *userdata);
```

`event_manager` subscribe functions return `event_handle`. `cmd_manager` and
`zngine_*_command_execute` keep their name-based signatures, which never leaked
pointers.

## Files affected

**ngine.lib** — `command.h`, `event.h`, `action.h`

**ngine.core** — `action_manager.h`, `event_manager.h`, `zod_ngine.h`,
`internal/action_manager/action_manager.c`,
`internal/event_manager/event_manager.c`, `internal/event_manager/sys_event.c`,
`internal/event_manager/event_manager_internal.h`,
`internal/zod_ngine/zod_ngine_action.c`, `internal/zod_ngine/zod_ngine.c`,
`internal/cmd_manager/sys_cmd.c`

**ngine.ext.console** — `internal/console/console.c`

**ngine.example** — `engine_run_example.c`, `beatup.c`

**Tests** — `ngine.lib/test_command.c`, `ngine.lib/test_event.c`,
`ngine.lib/test_action.c`, `ngine.core/test/test_cmd_manager.c`,
`ngine.ext.console/test/test_console.c`

## Testing

Existing tests are ported to the handle API rather than rewritten. Beyond that
port, the following cases are added, one set per module:

1. **Round trip.** Register or bind an entry, receive a handle, pass it through
   at least two further public functions, and confirm the observed effect. No
   `command *`, `action *`, or listener pointer appears in the test body.
2. **Invalid handle.** `*_execute` is called with `*_HANDLE_INVALID` and with an
   out-of-range handle. Each returns the documented error value and does not
   crash.
3. **Handle 0 default.** A zero-initialised handle variable is rejected by every
   resolver.
4. **Documented invalidation.** Bind two entries, remove the first, and assert
   that re-resolving by name or key yields a *different* handle than before.
   This pins the documented behaviour rather than asserting the hazard away.
5. **Group encoding, command only.** A system command and a user-defined command
   registered with the same name but different handlers produce distinct
   handles, and `command_execute` on each handle dispatches to the correct
   handler.
6. **Boundary check.** A build- or script-level assertion that the public
   section of each of the three headers — the text before `#ifdef
   *_IMPLEMENTATION` — contains no definition of `struct command`, `struct
   action`, or the event listener record, and mentions none of those types in a
   function signature.

The full existing suite (`test_command.out`, `test_event.out`,
`test_action.out`, `test_cmd_manager.out`, `test_console.out`) must pass, and
`engine_run` and `beatup_run` must still build and launch.
