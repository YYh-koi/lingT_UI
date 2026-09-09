# ATBM WiFi Synchronous Initialization Implementation Plan

> Execute this plan in the current session. Keep the public API small and preserve the existing AP/STA/P2P configuration behavior.

**Goal:** Add an explicit synchronous `atbm_wifi_init()` API, require it before starting a WiFi mode, and remove the P2P GO startup race.

**Architecture:** The public interface owns common SDIO/LwIP/driver readiness under its existing mutex. P2P keeps its worker task, but the lower-layer start function blocks on a binary completion event until `atbm_p2p_init()` reports success or failure. The application calls common initialization before its P2P mode wrapper.

**Tech Stack:** C, FreeRTOS binary semaphores, PowerShell contract tests, ARM GCC archive build.

---

### Task 1: Lock down the synchronous lifecycle contract

**Files:**
- Modify: `tools/wireless_lib/tests/test_atbm_wifi_interface.ps1`
- Modify: `tools/wireless_lib/tests/test_atbm_p2p_application_api.ps1`

1. Add assertions for the public `atbm_wifi_init()` declaration and implementation.
2. Assert that mode preparation checks `driver_ready` and no longer calls platform initialization.
3. Assert that `main.c` calls `atbm_wifi_init()` before the P2P mode wrapper.
4. Assert that P2P task initialization records a result, signals a completion event, and lower-layer start waits before returning.
5. Run both tests and confirm the new assertions fail before production changes.

### Task 2: Implement explicit common initialization

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/atbm_wifi.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_interface.c`

1. Declare `int atbm_wifi_init(void);` in both delivered public headers.
2. Implement the API using the existing interface mutex and `atbm_wifi_platform_init_locked()`.
3. Keep it idempotent by relying on the existing readiness flags.
4. Make mode preparation return `ATBM_WIFI_ERR_NOT_READY` unless common initialization has completed.
5. Keep deinitialization behavior compatible with reinitialization.

### Task 3: Make P2P initialization completion synchronous

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/hal/include/atbm_hal.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/net/wpa/p2p/p2p_main.c`

1. Add P2P initialization result/event state to the VIF.
2. Initialize the event before creating the P2P task and validate queue/task creation.
3. Store the `atbm_p2p_init()` result and signal the event from `p2p_main`.
4. Wait with a bounded timeout inside `atbm_p2p_start()` and return only after success.
5. Clean up task/queue/event resources on failure and normal P2P stop.

### Task 4: Update the application startup sequence

**Files:**
- Modify: `Project/GPCV2178_Turnkey_SDK/src/main.c`

1. Add the public initialization declaration in the existing ATBM extern block.
2. Call `atbm_wifi_init()` before `atbm_wifi_p2p_mode_enable()`.
3. Stop and log the error if common initialization fails; do not start dependent Bluetooth/socket/RTSP services.

### Task 5: Verify and deliver the WiFi archive

**Files:**
- Verify: `tools/wireless_lib/tests/*.ps1`
- Build: `tools/wireless_lib/build_wireless_libs.ps1`
- Update: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a`

1. Run the focused lifecycle/P2P contract tests.
2. Run the existing ATBM and library-tool test suite.
3. Build all 96 WiFi objects and overwrite the official `libatbm_wifi.a`.
4. Confirm archive existence, object count, timestamp, size, and SHA-256.

