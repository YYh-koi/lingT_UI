# AVRCP CT/TG Facade Split Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development and superpowers:verification-before-completion while executing this plan.

**Goal:** Split the combined AVRCP facade into a shared session layer, a Controller (CT) role module, and a Target (TG) role module without changing the public API, events, tasks, queues, or runtime behavior.

**Architecture:** `btstack_avrcp.c` remains the aggregate public facade and sole owner of connection state, peer address, AVRCP CID, and application event callback. `btstack_avrcp_controller.c` owns controller commands, metadata scheduling, controller notifications, and controller SDP setup. `btstack_avrcp_target.c` owns target responses, target-side volume handling, and target SDP setup. A private internal header exposes narrow session access and event delivery helpers; CT/TG do not own duplicate session state.

**Tech Stack:** C99, BTstack AVRCP classic APIs, PowerShell contract tests, G+ ARM GCC project generator/build.

---

### Task 1: Establish the split contract

**Files:**
- Modify: `btstack/profiles/tests/check_btstack_avrcp_profile.ps1`

1. Require the common, CT, TG, and private internal files.
2. Assert common owns connection/CID/public aggregate API only.
3. Assert controller-only APIs and target-only APIs are isolated to their role files.
4. Run the contract and confirm RED because CT/TG files do not yet exist.

### Task 2: Split the implementation

**Files:**
- Modify: `btstack/profiles/btstack_avrcp.c`
- Create: `btstack/profiles/btstack_avrcp_internal.h`
- Create: `btstack/profiles/btstack_avrcp_controller.c`
- Create: `btstack/profiles/btstack_avrcp_target.c`

1. Move controller handler, command dispatch, metadata/timer state, and controller SDP setup into the CT module.
2. Move target handler, target responses/volume handling, and target SDP setup into the TG module.
3. Keep base connection handler, CID/state/address, public callback, setup orchestration, connect/disconnect, and compatibility wrappers in common.
4. Route CT/TG events through the common internal emitter and read the shared CID through a narrow accessor.
5. Run the AVRCP/profile-manager focused contracts until GREEN.

### Task 3: Integrate all selected library builds

**Files:**
- Modify: `btstack/Makefile.inc`
- Modify: `Project/WirelessLibraries/BTstack_API_Library/BTstack_API_Library.gpcmprj`
- Modify: `Project/WirelessLibraries/BTstack_AT_Library/BTstack_AT_Library.gpcmprj`
- Modify: `Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj`
- Modify the wireless source manifest consumed by the Legacy generator if required.
- Regenerate the three generated Debug makefiles with the official generator.

1. Add CT and TG facade sources exactly once to the common source set and each selected project.
2. Regenerate project makefiles; do not hand-maintain generated source/rule lists.
3. Confirm the old combined object is still present once and the new CT/TG objects are each present once.

### Task 4: Verify compatibility

1. Run AVRCP, profile manager, event mapper, AT event, unified layout, and build integration contracts.
2. Run official API, AT, and Legacy builds.
3. Inspect archives for one common/CT/TG facade object each and verify no duplicate public symbols.
4. Report existing unrelated warnings separately; do not claim target interoperability without board testing.

No commit steps are included because this shared workspace is not managed as a Git working tree.
