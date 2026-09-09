# Core Dispatcher Naming Design

## Goal

Replace the ambiguous Core backend name with a dispatcher name that describes
request routing and execution without changing the public xStack API ABI.

## Naming

- Rename `btstack/core/btstack_backend.c` to
  `btstack/core/btstack_core_dispatcher.c`.
- Rename `btstack_core_backend_init()` to
  `btstack_core_dispatcher_init()`.
- Rename `btstack_core_backend_get_diagnostics()` to
  `btstack_core_dispatcher_get_diagnostics()`.
- Keep the concise request entry point `btstack_core_dispatch()`.
- Rename the Event Mapper-owned `btstack_core_backend_ready()` to
  `btstack_event_mapper_ready()`.

## Boundaries

Public `bt_xstack_*` declarations and ABI remain unchanged. Build manifests,
generated project makefiles, tests, archives, and the final firmware must use
only the new file and internal symbol names.

