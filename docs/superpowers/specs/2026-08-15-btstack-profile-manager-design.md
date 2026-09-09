# BTstack Profile Manager Design

## Goal

Replace the Classic-only composition module with a frontend-neutral Profile
Manager. The manager owns Classic profile lifecycle, aggregate Classic state,
normalized profile-event routing, and cross-profile coordination. Native
protocol parsing and state machines remain in their individual profile files.

## Scope

- Migrate HFP HF, A2DP Sink, AVRCP, and PBAP Client.
- Delete `btstack_classic_profiles.c` and its header.
- Preserve all public API and AT ABI.
- Preserve API/Event/STACK and AT/STACK worker models.
- Preserve direct A2DP and HFP audio callbacks.
- Reserve an LE configuration boundary, but require the LE mask to be zero.
- Do not refactor current GATT, HID, HOGP, GAP, or LE behavior.

## Ownership

Each profile owns native packet registration, native-event parsing, protocol
state machines, connection handles, and protocol cleanup. The Profile Manager
owns initialization order, normalized event subscriptions, aggregate Classic
state, event mapping/posting, and cross-profile rules. Event Mapper functions
for Classic profiles are stateless translations. The existing GAP/service
state remains transitional and is outside this migration.

## Initialization

`btstack_runtime_init()` obtains `btstack_profile_manager_config_t` from the
selected platform and calls `btstack_profile_manager_init()` after adapter
setup. The manager validates the full configuration before side effects,
initializes SDP/RFCOMM and enabled Classic profiles in the existing order,
wires normalized callbacks, applies Classic GAP identity/security settings,
and publishes READY only after all steps succeed.

The manager lifecycle is UNINITIALIZED, INITIALIZING, READY, or FAILED.
Identical initialization is idempotent. Concurrent initialization returns
BUSY. A different configuration after READY returns a configuration conflict.
A setup failure stops later setup, records FAILED, and lets runtime execute its
existing `late_deinit()` failure path.

## Configuration

The new config contains `struct_size`, common identity strings, a Classic
profile mask, a reserved LE mask, and the current Classic handles/channel/gain
fields. The LE mask must be zero in this phase. No runtime registry, weak
selection, dynamic allocation, or optional bare-metal path is introduced.

The selected platform callback is renamed from
`get_classic_profiles_config()` to `get_profile_manager_config()`.

## Event Flow

Native BTstack events are consumed by the corresponding profile. The profile
updates its internal state and emits its normalized internal event. The
Profile Manager updates its aggregate Classic snapshot, calls the matching
stateless Event Mapper conversion, and posts the resulting common envelope.

PBAP CONTACT/CALL post failure returns backpressure to the PBAP profile, which
retains responsibility for PAUSED/resume behavior. Audio sample/data callbacks
bypass Manager and Event Mapper.

## Cross-profile Coordination

The first and only migrated rule is HFP LOST -> PBAP peer disconnected. API or
AT output is delivered first. The frontend then calls
`btstack_profile_manager_event_delivered()`. The manager selects the internal
PBAP action, while the existing API and AT action sinks retain their proven
reliable delivery mechanisms. Profile Manager contains no RTOS, worker, or
ring implementation.

## Build and Compatibility

The new manager source is common and appears exactly once in API, AT, and
Legacy projects. The deleted Classic composition source/header and symbol are
absent. Generated makefiles and all three archives are rebuilt. Public headers,
public symbols, worker resources, ring capacities, and frontend mutual
exclusion remain unchanged.

## Verification

Host tests cover validation before side effects, initialization order,
idempotency, failure stop points, normalized event routing, aggregate state,
pure Classic mapping, PBAP backpressure, HFP LOST output-before-cleanup, and
API/AT reliable action paths. Contracts cover deleted files/symbols, selected
source sets, generated rules, archive membership, and no LE initialization.
Focused tests run before official API/AT/Legacy builds and final package checks.
