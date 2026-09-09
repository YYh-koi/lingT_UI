# BTstack Adapter Common Sources Move Design

## Goal

Move the platform-neutral adapter implementation sources and supporting
metadata into `btstack/core`/`btstack` without changing runtime behavior, then
remove the obsolete `btstack/adapter` directory completely.

## Scope

- Move `btstack_adapter.c`, `btstack_port_ops.c`, and `btstack_property.c`
  from `btstack/adapter` to `btstack/core` without modifying their APIs.
- Move the common source manifest and README to `btstack/Makefile.inc` and
  `btstack/README.md`, and move the retained contracts to `btstack/core/tests`.
- Keep the existing `include/adapter` headers unchanged to avoid a public
  include-path migration in this refactor.
- Update active G+ project files, generated make metadata, source manifests,
  and focused contracts to use the new source paths exactly once.
- Remove stale references to the already absent `bt_xstack_manager.c` where
  they are part of the same adapter source manifests/contracts.
- Delete the empty manager-internal header, obsolete manager contract, and the
  now-empty `btstack/adapter` directory.

## Architecture

The three moved sources remain common code. Platform-specific callbacks stay
implemented and registered by `btstack/ports/gpcv2178`. `btstack_port_ops_t`
continues to be the dependency-inversion boundary between core code and the
target port; only its implementation file location changes.

## Verification

A layout contract must fail before the move and pass afterward. The official
BTstack API project generator/build must then compile the new paths, package a
fresh archive, and existing focused common-layer/API contracts must remain
green. Active project and generated make files must not contain a
`btstack/adapter` include or source path.
