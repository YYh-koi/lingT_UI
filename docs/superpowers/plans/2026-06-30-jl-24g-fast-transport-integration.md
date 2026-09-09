# JL 2.4G Fast Transport Integration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Integrate the JL BLE-simulated 2.4G high-rate channel as a low-latency bearer for JLA audio while keeping HID on the normal HID/HOGP path and exposing one clean transport API to the product protocol stack.

**Architecture:** Treat JL 2.4G high-rate as a bearer below the product protocol stack, not as a generic BLE GATT pipe. The JL/radio firmware side owns BLE advertising, scanning, connection events, `le_high_rate_transport_instance()`, and `le_hw_send_packet_fast()`. The Generalplus/product side talks to that bearer through a small `ll_bearer` adapter and receives state/RX/TX events through a task-safe queue.

**Tech Stack:** JL BTStack high-rate demo, JL HOGP/HID APIs, Generalplus GPCV2178 FreeRTOS application, `msg_manager`, optional ATBM BLE/HCI bridge for reference only.

---

## Current Understanding

The JL source shows this split:

- JLA/low-latency audio data uses `le_high_rate_transport_instance()->send_data()`.
- That `send_data()` writes to a cbuffer first.
- Actual air transmission happens in `ble_event_irq_hook()`.
- `ble_event_irq_hook()` calls `le_high_rate_send_packet()`.
- `le_high_rate_send_packet()` calls `le_hw_send_packet_fast()`.
- HID report does not directly call `le_hw_send_packet_fast()` in the visible source.
- HID report goes through `hal_bt_hid_tx()` and then HOGP/HIDS or classic HID.

Important JL reference files:

- Source reference: `D:/Project/JL/Proj/BTStack/SDK/apps/dongle/hci/demo/le_high_rate_demo.c`
- Source reference: `D:/Project/JL/Proj/BTStack/SDK/apps/dongle/hci/demo/le_high_rate_demo.h`
- Source reference: `D:/Project/JL/Proj/BTStack/SDK/apps/dongle/hci/demo/btstack_demo.c`
- Source reference: `D:/Project/JL/Proj/BTStack/SDK/apps/dongle/hci/demo/btstack_demo.h`
- Source reference: `D:/Project/JL/Proj/BTStack/SDK/third_party/btstack/api/hal_bt.c`
- Source reference: `D:/Project/JL/Proj/BTStack/SDK/apps/dongle/hci/mode/bt/bt_key_msg_table.c`

Important Generalplus/ATBM reference files:

- Target reference: `D:/Project/Platform/Generalplus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/atbm_rtos_wifi6/hal/atbm_ble.c`
- Target reference: `D:/Project/Platform/Generalplus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/atbm_rtos_wifi6/hal/atbm_ble_driver_api.c`
- Target reference: `D:/Project/Platform/Generalplus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/application/msg_manager/src/msg_manager.c`
- Target reference: `D:/Project/Platform/Generalplus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/application/msg_manager/inc/msg_manager.h`
- Target reference: `D:/Project/Platform/Generalplus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`

## Non-Negotiable Design Rules

- Do not mix HID reports into the JLA fast packet unless both ends deliberately implement a new private multiplexing protocol.
- Do not expose `le_high_rate_transport_instance()` all over the product protocol stack.
- Do not call heavy protocol parsing from the JL fast RX callback.
- Do not assume `BT_BLE_RF` works unless the build includes `BIT_ENUM(TR_BLE_RF)`.
- Do not call `le_high_rate_init()` after BLE advertising or create-connection is already active.
- Do not expect a standard BLE peer to interoperate with the 1.25 ms high-rate mode.

## Proposed Layering

```text
Product protocol stack
  |
  |-- JLA audio / low-latency custom data
  |       -> ll_bearer_send(LL_BEARER_CH_AUDIO, ...)
  |       -> JL high-rate backend
  |       -> le_high_rate_transport_instance()->send_data()
  |       -> le_hw_send_packet_fast()
  |
  |-- HID report
  |       -> ll_bearer_send_hid(...)
  |       -> JL HID/HOGP backend
  |       -> hal_bt_hid_tx()
  |
  |-- BLE/GATT control, pairing, diagnostics
          -> existing BLE/HCI/GATT path or vendor control commands
```

The product protocol stack should only see:

```c
typedef enum {
    LL_BEARER_ROLE_PERIPHERAL = 0,
    LL_BEARER_ROLE_CENTRAL = 1,
} ll_bearer_role_t;

typedef enum {
    LL_BEARER_STATE_OFF = 0,
    LL_BEARER_STATE_INIT,
    LL_BEARER_STATE_ADV_SCAN,
    LL_BEARER_STATE_CONNECTED,
    LL_BEARER_STATE_READY,
    LL_BEARER_STATE_STREAMING,
    LL_BEARER_STATE_CLOSING,
} ll_bearer_state_t;

typedef enum {
    LL_BEARER_EVT_CONNECTED = 1,
    LL_BEARER_EVT_READY,
    LL_BEARER_EVT_RX,
    LL_BEARER_EVT_TX_DONE,
    LL_BEARER_EVT_DISCONNECTED,
    LL_BEARER_EVT_ERROR,
} ll_bearer_evt_t;

typedef enum {
    LL_BEARER_CH_AUDIO = 1,
    LL_BEARER_CH_CONTROL = 2,
    LL_BEARER_CH_DEBUG = 3,
} ll_bearer_channel_t;

typedef void (*ll_bearer_event_cb_t)(ll_bearer_evt_t evt,
                                     const unsigned char *data,
                                     unsigned short len,
                                     void *priv);

typedef struct {
    ll_bearer_role_t role;
    unsigned short frame_len;
    unsigned int pair_code;
    ll_bearer_event_cb_t event_cb;
    void *event_priv;
} ll_bearer_config_t;

typedef struct {
    int (*init)(const ll_bearer_config_t *cfg);
    int (*start)(void);
    int (*open)(unsigned short frame_len);
    int (*send)(ll_bearer_channel_t ch, const unsigned char *data, unsigned short len);
    int (*send_hid)(const unsigned char *report, unsigned short len);
    int (*close)(void);
    ll_bearer_state_t (*state)(void);
} ll_bearer_ops_t;
```

## File Structure

### Product Protocol Abstraction

- Create: `D:/Project/Platform/Generalplus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/application/ll_bearer/inc/ll_bearer.h`
  - Owns product-facing transport types and APIs.
- Create: `D:/Project/Platform/Generalplus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/application/ll_bearer/src/ll_bearer.c`
  - Owns backend selection, state checks, and dispatch to JL or stub backend.
- Create: `D:/Project/Platform/Generalplus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/application/ll_bearer/inc/ll_bearer_jl24g.h`
  - Owns JL high-rate backend declarations.
- Create: `D:/Project/Platform/Generalplus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/application/ll_bearer/src/ll_bearer_jl24g.c`
  - Owns Generalplus-side JL control/data adapter. This file must not directly call `le_hw_send_packet_fast()` unless the code is compiled inside JL firmware.
- Create: `D:/Project/Platform/Generalplus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/application/ll_bearer/src/ll_bearer_stub.c`
  - Owns compile-only fallback and host-side fake RX/TX tests.
- Create: `D:/Project/Platform/Generalplus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/application/ll_bearer/inc/ll_bearer_jl_ctrl.h`
  - Owns optional host-to-JL vendor command/event frame definitions.

### JL Firmware Backend

If the product protocol stack is compiled inside the JL firmware, create these in the JL tree:

- Create: `D:/Project/JL/Proj/BTStack/SDK/apps/dongle/hci/ll_bearer/jl24g_radio.h`
- Create: `D:/Project/JL/Proj/BTStack/SDK/apps/dongle/hci/ll_bearer/jl24g_radio.c`
  - Wraps `le_high_rate_init()`, `le_high_rate_transport_instance()`, RX callback registration, and HID split.

If the product protocol stack is compiled on the Generalplus host and controls a separate JL radio firmware, create these in the JL tree instead:

- Create: `D:/Project/JL/Proj/BTStack/SDK/apps/dongle/hci/ll_bearer/jl24g_vendor_cmd.h`
- Create: `D:/Project/JL/Proj/BTStack/SDK/apps/dongle/hci/ll_bearer/jl24g_vendor_cmd.c`
  - Implements host commands such as INIT, START_ADV, START_SCAN, OPEN, AUDIO_TX, HID_TX, CLOSE.
  - Emits host events such as CONNECTED, READY, RX, TX_DONE, DISCONNECTED, ERROR.

### Build Integration

- Modify: `D:/Project/Platform/Generalplus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`
  - Add `application/ll_bearer/src/*.c` files.
  - Add include path `application/ll_bearer/inc`.
- Modify: `D:/Project/Platform/Generalplus/GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm/Project/Include/project.h`
  - Add feature macro defaults if this is where product-level switches live.

Suggested feature macros:

```c
#ifndef CONFIG_LL_BEARER_ENABLE
#define CONFIG_LL_BEARER_ENABLE 1
#endif

#ifndef CONFIG_LL_BEARER_JL24G_ENABLE
#define CONFIG_LL_BEARER_JL24G_ENABLE 1
#endif

#ifndef CONFIG_LL_BEARER_STUB_ENABLE
#define CONFIG_LL_BEARER_STUB_ENABLE 1
#endif

#ifndef CONFIG_LL_BEARER_DEFAULT_FRAME_LEN
#define CONFIG_LL_BEARER_DEFAULT_FRAME_LEN 100
#endif

#ifndef CONFIG_LL_BEARER_MAX_FAST_PAYLOAD
#define CONFIG_LL_BEARER_MAX_FAST_PAYLOAD 214
#endif
```

---

### Task 1: Add Product-Facing Bearer API

**Files:**
- Create: `application/ll_bearer/inc/ll_bearer.h`
- Create: `application/ll_bearer/src/ll_bearer.c`
- Create: `application/ll_bearer/src/ll_bearer_stub.c`
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`

- [ ] **Step 1: Add `ll_bearer.h` with stable product-facing types**

```c
#ifndef LL_BEARER_H
#define LL_BEARER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LL_BEARER_ROLE_PERIPHERAL = 0,
    LL_BEARER_ROLE_CENTRAL = 1,
} ll_bearer_role_t;

typedef enum {
    LL_BEARER_STATE_OFF = 0,
    LL_BEARER_STATE_INIT,
    LL_BEARER_STATE_ADV_SCAN,
    LL_BEARER_STATE_CONNECTED,
    LL_BEARER_STATE_READY,
    LL_BEARER_STATE_STREAMING,
    LL_BEARER_STATE_CLOSING,
} ll_bearer_state_t;

typedef enum {
    LL_BEARER_EVT_CONNECTED = 1,
    LL_BEARER_EVT_READY,
    LL_BEARER_EVT_RX,
    LL_BEARER_EVT_TX_DONE,
    LL_BEARER_EVT_DISCONNECTED,
    LL_BEARER_EVT_ERROR,
} ll_bearer_evt_t;

typedef enum {
    LL_BEARER_CH_AUDIO = 1,
    LL_BEARER_CH_CONTROL = 2,
    LL_BEARER_CH_DEBUG = 3,
} ll_bearer_channel_t;

typedef void (*ll_bearer_event_cb_t)(ll_bearer_evt_t evt,
                                     const unsigned char *data,
                                     unsigned short len,
                                     void *priv);

typedef struct {
    ll_bearer_role_t role;
    unsigned short frame_len;
    unsigned int pair_code;
    ll_bearer_event_cb_t event_cb;
    void *event_priv;
} ll_bearer_config_t;

int ll_bearer_init(const ll_bearer_config_t *cfg);
int ll_bearer_start(void);
int ll_bearer_open(unsigned short frame_len);
int ll_bearer_send(ll_bearer_channel_t ch, const unsigned char *data, unsigned short len);
int ll_bearer_send_hid(const unsigned char *report, unsigned short len);
int ll_bearer_close(void);
ll_bearer_state_t ll_bearer_get_state(void);

#ifdef __cplusplus
}
#endif

#endif
```

- [ ] **Step 2: Add backend interface inside `ll_bearer.c`**

```c
#include "ll_bearer.h"

typedef struct {
    int (*init)(const ll_bearer_config_t *cfg);
    int (*start)(void);
    int (*open)(unsigned short frame_len);
    int (*send)(ll_bearer_channel_t ch, const unsigned char *data, unsigned short len);
    int (*send_hid)(const unsigned char *report, unsigned short len);
    int (*close)(void);
    ll_bearer_state_t (*state)(void);
} ll_bearer_backend_t;

extern const ll_bearer_backend_t ll_bearer_jl24g_backend;
extern const ll_bearer_backend_t ll_bearer_stub_backend;

static const ll_bearer_backend_t *s_backend;
static ll_bearer_config_t s_cfg;

int ll_bearer_init(const ll_bearer_config_t *cfg)
{
    if (cfg == 0 || cfg->frame_len == 0) {
        return -1;
    }

    s_cfg = *cfg;

#if CONFIG_LL_BEARER_JL24G_ENABLE
    s_backend = &ll_bearer_jl24g_backend;
#else
    s_backend = &ll_bearer_stub_backend;
#endif

    return s_backend->init(&s_cfg);
}

int ll_bearer_start(void)
{
    return s_backend ? s_backend->start() : -1;
}

int ll_bearer_open(unsigned short frame_len)
{
    return s_backend ? s_backend->open(frame_len) : -1;
}

int ll_bearer_send(ll_bearer_channel_t ch, const unsigned char *data, unsigned short len)
{
    if (s_backend == 0 || data == 0 || len == 0) {
        return -1;
    }
    return s_backend->send(ch, data, len);
}

int ll_bearer_send_hid(const unsigned char *report, unsigned short len)
{
    if (s_backend == 0 || report == 0 || len == 0) {
        return -1;
    }
    return s_backend->send_hid(report, len);
}

int ll_bearer_close(void)
{
    return s_backend ? s_backend->close() : -1;
}

ll_bearer_state_t ll_bearer_get_state(void)
{
    return s_backend ? s_backend->state() : LL_BEARER_STATE_OFF;
}
```

- [ ] **Step 3: Add stub backend for compile and protocol-stack smoke tests**

```c
#include "ll_bearer.h"

typedef struct {
    int (*init)(const ll_bearer_config_t *cfg);
    int (*start)(void);
    int (*open)(unsigned short frame_len);
    int (*send)(ll_bearer_channel_t ch, const unsigned char *data, unsigned short len);
    int (*send_hid)(const unsigned char *report, unsigned short len);
    int (*close)(void);
    ll_bearer_state_t (*state)(void);
} ll_bearer_backend_t;

static ll_bearer_config_t s_stub_cfg;
static ll_bearer_state_t s_stub_state = LL_BEARER_STATE_OFF;

static int stub_init(const ll_bearer_config_t *cfg)
{
    s_stub_cfg = *cfg;
    s_stub_state = LL_BEARER_STATE_INIT;
    return 0;
}

static int stub_start(void)
{
    s_stub_state = LL_BEARER_STATE_ADV_SCAN;
    if (s_stub_cfg.event_cb) {
        s_stub_cfg.event_cb(LL_BEARER_EVT_CONNECTED, 0, 0, s_stub_cfg.event_priv);
        s_stub_cfg.event_cb(LL_BEARER_EVT_READY, 0, 0, s_stub_cfg.event_priv);
    }
    s_stub_state = LL_BEARER_STATE_READY;
    return 0;
}

static int stub_open(unsigned short frame_len)
{
    (void)frame_len;
    s_stub_state = LL_BEARER_STATE_STREAMING;
    return 0;
}

static int stub_send(ll_bearer_channel_t ch, const unsigned char *data, unsigned short len)
{
    (void)ch;
    if (s_stub_cfg.event_cb) {
        s_stub_cfg.event_cb(LL_BEARER_EVT_TX_DONE, data, len, s_stub_cfg.event_priv);
    }
    return len;
}

static int stub_send_hid(const unsigned char *report, unsigned short len)
{
    if (s_stub_cfg.event_cb) {
        s_stub_cfg.event_cb(LL_BEARER_EVT_TX_DONE, report, len, s_stub_cfg.event_priv);
    }
    return len;
}

static int stub_close(void)
{
    s_stub_state = LL_BEARER_STATE_OFF;
    if (s_stub_cfg.event_cb) {
        s_stub_cfg.event_cb(LL_BEARER_EVT_DISCONNECTED, 0, 0, s_stub_cfg.event_priv);
    }
    return 0;
}

static ll_bearer_state_t stub_state(void)
{
    return s_stub_state;
}

const ll_bearer_backend_t ll_bearer_stub_backend = {
    stub_init,
    stub_start,
    stub_open,
    stub_send,
    stub_send_hid,
    stub_close,
    stub_state,
};
```

- [ ] **Step 4: Add the new files to the Generalplus project**

Add these units to `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`:

```xml
<Unit filename="../../application/ll_bearer/src/ll_bearer.c">
</Unit>
<Unit filename="../../application/ll_bearer/src/ll_bearer_stub.c">
</Unit>
<Unit filename="../../application/ll_bearer/src/ll_bearer_jl24g.c">
</Unit>
```

Add include path:

```text
../../application/ll_bearer/inc
```

- [ ] **Step 5: Compile with only stub backend**

Set:

```c
#define CONFIG_LL_BEARER_ENABLE 1
#define CONFIG_LL_BEARER_JL24G_ENABLE 0
#define CONFIG_LL_BEARER_STUB_ENABLE 1
```

Expected: project compiles without needing JL headers or JL libraries.

---

### Task 2: Add Generalplus-Side JL 2.4G Backend Shell

**Files:**
- Create: `application/ll_bearer/inc/ll_bearer_jl24g.h`
- Create: `application/ll_bearer/inc/ll_bearer_jl_ctrl.h`
- Create: `application/ll_bearer/src/ll_bearer_jl24g.c`

- [ ] **Step 1: Define host-to-JL control frames**

```c
#ifndef LL_BEARER_JL_CTRL_H
#define LL_BEARER_JL_CTRL_H

#define JL24G_CTRL_MAGIC0 0x4a
#define JL24G_CTRL_MAGIC1 0x32
#define JL24G_CTRL_VERSION 1

typedef enum {
    JL24G_CMD_INIT = 1,
    JL24G_CMD_START = 2,
    JL24G_CMD_OPEN = 3,
    JL24G_CMD_AUDIO_TX = 4,
    JL24G_CMD_HID_TX = 5,
    JL24G_CMD_CLOSE = 6,
} jl24g_cmd_t;

typedef enum {
    JL24G_EVT_CONNECTED = 1,
    JL24G_EVT_READY = 2,
    JL24G_EVT_RX = 3,
    JL24G_EVT_TX_DONE = 4,
    JL24G_EVT_DISCONNECTED = 5,
    JL24G_EVT_ERROR = 6,
} jl24g_evt_t;

typedef struct {
    unsigned char magic0;
    unsigned char magic1;
    unsigned char version;
    unsigned char id;
    unsigned short len;
    unsigned short seq;
} jl24g_ctrl_hdr_t;

#endif
```

- [ ] **Step 2: Add transport hooks that hide the physical host-link**

The first version should compile even before the real USB/UART/HCI vendor command layer is ready.

```c
#ifndef LL_BEARER_JL24G_H
#define LL_BEARER_JL24G_H

int jl24g_host_link_tx(const unsigned char *data, unsigned short len);
void jl24g_host_link_on_rx(const unsigned char *data, unsigned short len);

#endif
```

- [ ] **Step 3: Implement `ll_bearer_jl24g.c` as a host-side adapter**

This file must package commands to the JL radio side. It must not directly call `le_hw_send_packet_fast()` in the Generalplus build.

```c
#include "ll_bearer.h"
#include "ll_bearer_jl24g.h"
#include "ll_bearer_jl_ctrl.h"

typedef struct {
    int (*init)(const ll_bearer_config_t *cfg);
    int (*start)(void);
    int (*open)(unsigned short frame_len);
    int (*send)(ll_bearer_channel_t ch, const unsigned char *data, unsigned short len);
    int (*send_hid)(const unsigned char *report, unsigned short len);
    int (*close)(void);
    ll_bearer_state_t (*state)(void);
} ll_bearer_backend_t;

static ll_bearer_config_t s_cfg;
static ll_bearer_state_t s_state = LL_BEARER_STATE_OFF;
static unsigned short s_seq;

static int send_cmd(unsigned char cmd, const unsigned char *payload, unsigned short len)
{
    unsigned char frame[8 + CONFIG_LL_BEARER_MAX_FAST_PAYLOAD];
    jl24g_ctrl_hdr_t *hdr = (jl24g_ctrl_hdr_t *)frame;

    if (len > CONFIG_LL_BEARER_MAX_FAST_PAYLOAD) {
        return -1;
    }

    hdr->magic0 = JL24G_CTRL_MAGIC0;
    hdr->magic1 = JL24G_CTRL_MAGIC1;
    hdr->version = JL24G_CTRL_VERSION;
    hdr->id = cmd;
    hdr->len = len;
    hdr->seq = ++s_seq;

    if (payload && len) {
        memcpy(frame + sizeof(*hdr), payload, len);
    }

    return jl24g_host_link_tx(frame, (unsigned short)(sizeof(*hdr) + len));
}

static int jl24g_init(const ll_bearer_config_t *cfg)
{
    if (cfg == 0) {
        return -1;
    }
    s_cfg = *cfg;
    s_state = LL_BEARER_STATE_INIT;
    return send_cmd(JL24G_CMD_INIT, (const unsigned char *)cfg, sizeof(*cfg));
}

static int jl24g_start(void)
{
    s_state = LL_BEARER_STATE_ADV_SCAN;
    return send_cmd(JL24G_CMD_START, 0, 0);
}

static int jl24g_open(unsigned short frame_len)
{
    unsigned char payload[2];
    payload[0] = (unsigned char)(frame_len & 0xff);
    payload[1] = (unsigned char)(frame_len >> 8);
    return send_cmd(JL24G_CMD_OPEN, payload, sizeof(payload));
}

static int jl24g_send(ll_bearer_channel_t ch, const unsigned char *data, unsigned short len)
{
    unsigned char frame[1 + CONFIG_LL_BEARER_MAX_FAST_PAYLOAD];
    if (data == 0 || len == 0 || len > CONFIG_LL_BEARER_MAX_FAST_PAYLOAD) {
        return -1;
    }
    frame[0] = (unsigned char)ch;
    memcpy(&frame[1], data, len);
    return send_cmd(JL24G_CMD_AUDIO_TX, frame, (unsigned short)(len + 1));
}

static int jl24g_send_hid(const unsigned char *report, unsigned short len)
{
    if (report == 0 || len == 0) {
        return -1;
    }
    return send_cmd(JL24G_CMD_HID_TX, report, len);
}

static int jl24g_close(void)
{
    s_state = LL_BEARER_STATE_CLOSING;
    return send_cmd(JL24G_CMD_CLOSE, 0, 0);
}

static ll_bearer_state_t jl24g_state(void)
{
    return s_state;
}

void jl24g_host_link_on_rx(const unsigned char *data, unsigned short len)
{
    const jl24g_ctrl_hdr_t *hdr;
    const unsigned char *payload;
    unsigned short payload_len;

    if (data == 0 || len < sizeof(jl24g_ctrl_hdr_t)) {
        return;
    }

    hdr = (const jl24g_ctrl_hdr_t *)data;
    if (hdr->magic0 != JL24G_CTRL_MAGIC0 || hdr->magic1 != JL24G_CTRL_MAGIC1) {
        return;
    }

    payload = data + sizeof(*hdr);
    payload_len = hdr->len;

    switch ((jl24g_evt_t)hdr->id) {
    case JL24G_EVT_CONNECTED:
        s_state = LL_BEARER_STATE_CONNECTED;
        break;
    case JL24G_EVT_READY:
        s_state = LL_BEARER_STATE_READY;
        break;
    case JL24G_EVT_DISCONNECTED:
        s_state = LL_BEARER_STATE_OFF;
        break;
    default:
        break;
    }

    if (s_cfg.event_cb) {
        s_cfg.event_cb((ll_bearer_evt_t)hdr->id, payload, payload_len, s_cfg.event_priv);
    }
}

__attribute__((weak))
int jl24g_host_link_tx(const unsigned char *data, unsigned short len)
{
    (void)data;
    (void)len;
    return -1;
}

const ll_bearer_backend_t ll_bearer_jl24g_backend = {
    jl24g_init,
    jl24g_start,
    jl24g_open,
    jl24g_send,
    jl24g_send_hid,
    jl24g_close,
    jl24g_state,
};
```

- [ ] **Step 4: Compile with `CONFIG_LL_BEARER_JL24G_ENABLE=1`**

Expected: compile succeeds, but `jl24g_host_link_tx()` returns `-1` until the physical link is connected.

---

### Task 3: Add JL Firmware-Side Radio Wrapper

**Files:**
- Create: `SDK/apps/dongle/hci/ll_bearer/jl24g_radio.h`
- Create: `SDK/apps/dongle/hci/ll_bearer/jl24g_radio.c`
- Modify: `SDK/apps/dongle/hci/demo/btstack_demo.c`
- Modify: `SDK/apps/dongle/hci/demo/btstack_demo.h`

- [ ] **Step 1: Add JL-side wrapper header**

```c
#ifndef JL24G_RADIO_H
#define JL24G_RADIO_H

#include "typedef.h"

typedef enum {
    JL24G_ROLE_PERIPHERAL = 0,
    JL24G_ROLE_CENTRAL = 1,
} jl24g_role_t;

typedef void (*jl24g_rx_cb_t)(void *priv, const u8 *data, u32 len);
typedef void (*jl24g_tx_cb_t)(void *priv, const u8 *data, u32 len);

typedef struct {
    jl24g_role_t role;
    u16 frame_len;
    u32 pair_code;
    jl24g_rx_cb_t rx_cb;
    jl24g_tx_cb_t tx_cb;
    void *priv;
} jl24g_radio_config_t;

int jl24g_radio_init(const jl24g_radio_config_t *cfg);
int jl24g_radio_start(void);
int jl24g_radio_open(u16 frame_len);
int jl24g_radio_send_audio(const u8 *data, u16 len);
int jl24g_radio_send_hid(const u8 *report, u16 len);
int jl24g_radio_close(void);
void jl24g_radio_on_ble_ready(void);
void jl24g_radio_on_ble_disconnect(void);

#endif
```

- [ ] **Step 2: Add JL-side implementation**

```c
#include "jl24g_radio.h"
#include "le_high_rate_demo.h"
#include "btstack/bluetooth.h"
#include "third_party/btstack/api/hal_bt.h"

static jl24g_radio_config_t s_cfg;
static const le_high_rate_transport_t *s_ops;
static u8 s_ready;

static void jl24g_rx_bridge(void *priv, const u8 *data, u32 len)
{
    if (s_cfg.rx_cb) {
        s_cfg.rx_cb(priv, data, len);
    }
}

static void jl24g_tx_bridge(void *priv, const u8 *data, u32 len)
{
    if (s_cfg.tx_cb) {
        s_cfg.tx_cb(priv, data, len);
    }
}

int jl24g_radio_init(const jl24g_radio_config_t *cfg)
{
    if (cfg == 0 || cfg->frame_len == 0) {
        return -1;
    }
    s_cfg = *cfg;

    le_high_rate_init();
    s_ops = le_high_rate_transport_instance();
    if (s_ops == 0) {
        return -1;
    }

    s_ops->register_rx_callback(s_cfg.priv, jl24g_rx_bridge);
    s_ops->register_tx_callback(s_cfg.priv, jl24g_tx_bridge);
    return 0;
}

int jl24g_radio_start(void)
{
    return 0;
}

void jl24g_radio_on_ble_ready(void)
{
    if (s_ops && !s_ready) {
        s_ops->open(s_cfg.frame_len);
        s_ready = 1;
    }
}

int jl24g_radio_open(u16 frame_len)
{
    if (s_ops == 0 || frame_len == 0) {
        return -1;
    }
    s_cfg.frame_len = frame_len;
    s_ops->open(frame_len);
    s_ready = 1;
    return 0;
}

int jl24g_radio_send_audio(const u8 *data, u16 len)
{
    if (s_ops == 0 || !s_ready || data == 0 || len == 0) {
        return -1;
    }
    return s_ops->send_data((void *)data, len);
}

int jl24g_radio_send_hid(const u8 *report, u16 len)
{
    if (report == 0 || len == 0) {
        return -1;
    }
    return hal_bt_hid_tx(BT_ID0, BT_BLE, HID_REPORT_TYPE_INPUT, (u8 *)report, len) ? len : -1;
}

int jl24g_radio_close(void)
{
    le_high_rate_disconn_deinit();
    s_ready = 0;
    return 0;
}

void jl24g_radio_on_ble_disconnect(void)
{
    jl24g_radio_close();
}
```

- [ ] **Step 3: Call `jl24g_radio_init()` before advertising or create-connection**

Place the call at the same timing as the reference `le_high_rate_init()` calls in `btstack_demo.c`, before `hci_le_set_advertise_enable` on peripheral and before `hci_le_create_connection_with_params` on central.

Expected timing:

```text
boot
  -> configure role
  -> jl24g_radio_init()
  -> enable adv OR create connection
  -> HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE
  -> jl24g_radio_on_ble_ready()
```

- [ ] **Step 4: Call `jl24g_radio_on_ble_ready()` from LE ready event**

In `btstack_demo.c`, update the `HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE` case:

```c
case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
    jl24g_radio_on_ble_ready();
    break;
```

- [ ] **Step 5: Call `jl24g_radio_on_ble_disconnect()` from disconnect handling**

Expected: fast cbuffer is cleared, send flag is disabled, and upper layer receives DISCONNECTED.

---

### Task 4: Add Optional Host Vendor Command Bridge

**Files:**
- Create: `SDK/apps/dongle/hci/ll_bearer/jl24g_vendor_cmd.h`
- Create: `SDK/apps/dongle/hci/ll_bearer/jl24g_vendor_cmd.c`
- Modify: the JL HCI/vendor command dispatch file used by the product build
- Modify: the Generalplus file that owns the physical link to JL radio

- [ ] **Step 1: Add command handler on JL side**

```c
int jl24g_vendor_cmd_handle(const u8 *cmd, u16 len)
{
    const jl24g_ctrl_hdr_t *hdr = (const jl24g_ctrl_hdr_t *)cmd;
    const u8 *payload = cmd + sizeof(*hdr);

    if (len < sizeof(*hdr)) {
        return -1;
    }

    switch (hdr->id) {
    case JL24G_CMD_INIT:
        return jl24g_radio_init((const jl24g_radio_config_t *)payload);
    case JL24G_CMD_START:
        return jl24g_radio_start();
    case JL24G_CMD_OPEN:
        return jl24g_radio_open((u16)(payload[0] | (payload[1] << 8)));
    case JL24G_CMD_AUDIO_TX:
        return jl24g_radio_send_audio(payload + 1, (u16)(hdr->len - 1));
    case JL24G_CMD_HID_TX:
        return jl24g_radio_send_hid(payload, hdr->len);
    case JL24G_CMD_CLOSE:
        return jl24g_radio_close();
    default:
        return -1;
    }
}
```

- [ ] **Step 2: Add event emitter on JL side**

```c
static void jl24g_vendor_emit_event(u8 evt, const u8 *payload, u16 len)
{
    u8 frame[sizeof(jl24g_ctrl_hdr_t) + CONFIG_LL_BEARER_MAX_FAST_PAYLOAD];
    jl24g_ctrl_hdr_t *hdr = (jl24g_ctrl_hdr_t *)frame;

    hdr->magic0 = JL24G_CTRL_MAGIC0;
    hdr->magic1 = JL24G_CTRL_MAGIC1;
    hdr->version = JL24G_CTRL_VERSION;
    hdr->id = evt;
    hdr->len = len;
    hdr->seq++;

    if (payload && len) {
        memcpy(frame + sizeof(*hdr), payload, len);
    }

    jl_host_link_event_tx(frame, sizeof(*hdr) + len);
}
```

- [ ] **Step 3: Connect Generalplus physical link RX to `jl24g_host_link_on_rx()`**

Example:

```c
void jl_link_rx_dispatch(const unsigned char *buf, unsigned short len)
{
    if (len >= 2 && buf[0] == JL24G_CTRL_MAGIC0 && buf[1] == JL24G_CTRL_MAGIC1) {
        jl24g_host_link_on_rx(buf, len);
        return;
    }

    existing_hci_or_vendor_rx_dispatch(buf, len);
}
```

- [ ] **Step 4: Implement `jl24g_host_link_tx()`**

The real implementation depends on hardware:

- USB vendor endpoint
- UART vendor framing
- HCI vendor command
- shared mailbox

Required behavior:

```c
int jl24g_host_link_tx(const unsigned char *data, unsigned short len)
{
    if (data == 0 || len == 0) {
        return -1;
    }
    return product_jl_link_write(data, len);
}
```

Expected: `ll_bearer_init/start/open/send` reaches JL firmware and returns an ACK or state event.

---

### Task 5: Wire The Product Protocol Stack To `ll_bearer`

**Files:**
- Modify: the product protocol stack entry file that currently sends JLA/audio packets
- Modify: the product protocol stack file that currently sends HID reports
- Modify: the task or state machine that handles link events
- Use: `application/msg_manager/inc/msg_manager.h`

- [ ] **Step 1: Add a product protocol event shim**

```c
typedef enum {
    PROTO_LINK_MSG_CONNECTED = 0x2401,
    PROTO_LINK_MSG_READY,
    PROTO_LINK_MSG_RX,
    PROTO_LINK_MSG_TX_DONE,
    PROTO_LINK_MSG_DISCONNECTED,
    PROTO_LINK_MSG_ERROR,
} proto_link_msg_t;
```

- [ ] **Step 2: Keep `ll_bearer` callback small**

```c
static void proto_ll_evt_cb(ll_bearer_evt_t evt,
                            const unsigned char *data,
                            unsigned short len,
                            void *priv)
{
    proto_rx_item_t item;

    memset(&item, 0, sizeof(item));
    item.evt = evt;

    if (data && len && len <= sizeof(item.payload)) {
        memcpy(item.payload, data, len);
        item.len = len;
    }

    msgQSend(protoTaskQ, PROTO_LINK_MSG_RX, &item, sizeof(item), MSG_PRI_NORMAL);
}
```

- [ ] **Step 3: Initialize the bearer when the product protocol starts**

```c
static int proto_link_start(void)
{
    ll_bearer_config_t cfg;

    memset(&cfg, 0, sizeof(cfg));
    cfg.role = LL_BEARER_ROLE_PERIPHERAL;
    cfg.frame_len = CONFIG_LL_BEARER_DEFAULT_FRAME_LEN;
    cfg.pair_code = 0;
    cfg.event_cb = proto_ll_evt_cb;
    cfg.event_priv = 0;

    if (ll_bearer_init(&cfg) != 0) {
        return -1;
    }
    return ll_bearer_start();
}
```

- [ ] **Step 4: Send JLA through `ll_bearer_send()`**

```c
int proto_jla_send(const unsigned char *frame, unsigned short len)
{
    if (ll_bearer_get_state() < LL_BEARER_STATE_READY) {
        return -1;
    }
    return ll_bearer_send(LL_BEARER_CH_AUDIO, frame, len);
}
```

- [ ] **Step 5: Send HID through `ll_bearer_send_hid()`**

```c
int proto_hid_send(const unsigned char *report, unsigned short len)
{
    if (ll_bearer_get_state() < LL_BEARER_STATE_CONNECTED) {
        return -1;
    }
    return ll_bearer_send_hid(report, len);
}
```

Expected: product protocol code no longer knows about JL `le_high_rate_transport_instance()` or ATBM `ieee80211_ble_commb_xmit()`.

---

### Task 6: Add State Machine And Backpressure

**Files:**
- Modify: `application/ll_bearer/src/ll_bearer.c`
- Modify: `application/ll_bearer/src/ll_bearer_jl24g.c`

- [ ] **Step 1: Reject sends before READY**

```c
int ll_bearer_send(ll_bearer_channel_t ch, const unsigned char *data, unsigned short len)
{
    if (s_backend == 0 || data == 0 || len == 0) {
        return -1;
    }
    if (s_backend->state() < LL_BEARER_STATE_READY) {
        return -2;
    }
    if (len > CONFIG_LL_BEARER_MAX_FAST_PAYLOAD) {
        return -3;
    }
    return s_backend->send(ch, data, len);
}
```

- [ ] **Step 2: Use a defined drop policy for audio**

For JLA audio, low latency is more important than perfect delivery. Use this rule:

```text
If audio TX queue is full:
  Drop the oldest unsent audio frame.
  Keep the newest frame.
  Increment audio_tx_drop_count.
```

- [ ] **Step 3: Use a different policy for control**

For control packets:

```text
If control TX queue is full:
  Reject the new packet with -E_BUSY.
  Do not drop older control packets.
```

- [ ] **Step 4: Track counters**

```c
typedef struct {
    unsigned int audio_tx_ok;
    unsigned int audio_tx_drop;
    unsigned int audio_rx_ok;
    unsigned int hid_tx_ok;
    unsigned int hid_tx_fail;
    unsigned int link_disconnects;
    unsigned int link_errors;
} ll_bearer_stats_t;
```

Expected: serial logs can confirm drops and link health without adding temporary debug prints everywhere.

---

### Task 7: Build And Feature Macro Integration

**Files:**
- Modify: `Project/Include/project.h`
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`
- Modify: JL app config files if building inside JL tree

- [ ] **Step 1: Add Generalplus feature defaults**

```c
#ifndef CONFIG_LL_BEARER_ENABLE
#define CONFIG_LL_BEARER_ENABLE 1
#endif

#ifndef CONFIG_LL_BEARER_JL24G_ENABLE
#define CONFIG_LL_BEARER_JL24G_ENABLE 1
#endif

#ifndef CONFIG_LL_BEARER_STUB_ENABLE
#define CONFIG_LL_BEARER_STUB_ENABLE 1
#endif

#ifndef CONFIG_LL_BEARER_MAX_FAST_PAYLOAD
#define CONFIG_LL_BEARER_MAX_FAST_PAYLOAD 214
#endif
```

- [ ] **Step 2: Keep ATBM BLE off unless intentionally testing ATBM BLE**

Current target config has:

```c
#define CONFIG_ATBM_BLE_ADV_COEXIST 0
#define CONFIG_WIFI_BT_COMB 0
```

Keep this unchanged for JL 2.4G bring-up. ATBM BLE is a separate backend and should not be enabled just to test JL fast transport.

- [ ] **Step 3: Add JL-side build switches**

Use the JL demo defaults as reference:

```c
#define HIGH_REPORT_RATE_2SLOT 1
#define HIGH_RATE_TRANS_MODE HIGH_RATE_UPDOWNSTREAM_MODE
```

For role selection:

```c
#define TCFG_GAMEPAD_MODE TCFG_WIRELESS_GAMEPAD_ENABLE
```

or:

```c
#define TCFG_GAMEPAD_MODE TCFG_WIRELESS_GAMEPAD_RECEIVER_ENABLE
```

Peripheral side should advertise. Central side should scan/create connection.

---

### Task 8: Verification Plan

**Files:**
- Create: `tmp/tests/jl24g_static_check.ps1`
- Create: `tmp/tests/jl24g_plan_smoke_check.ps1`

- [ ] **Step 1: Add static source checks**

```powershell
$root = "D:\Project\Platform\Generalplus\GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm"
$files = @(
  "$root\application\ll_bearer\inc\ll_bearer.h",
  "$root\application\ll_bearer\src\ll_bearer.c",
  "$root\application\ll_bearer\src\ll_bearer_jl24g.c",
  "$root\application\ll_bearer\src\ll_bearer_stub.c"
)

foreach ($file in $files) {
  if (!(Test-Path $file)) {
    throw "Missing $file"
  }
}

$jl24g = Get-Content "$root\application\ll_bearer\src\ll_bearer_jl24g.c" -Raw
if ($jl24g -match "le_hw_send_packet_fast") {
  throw "Generalplus host adapter must not call le_hw_send_packet_fast directly"
}

Write-Host "jl24g static check passed"
```

- [ ] **Step 2: Compile with stub backend**

Expected:

```text
CONFIG_LL_BEARER_JL24G_ENABLE=0
Build succeeds.
Product protocol can start, reach READY, send fake JLA, and receive TX_DONE.
```

- [ ] **Step 3: Compile with JL backend shell**

Expected:

```text
CONFIG_LL_BEARER_JL24G_ENABLE=1
Build succeeds.
No direct JL controller symbols are required by the Generalplus build.
Only jl24g_host_link_tx remains to be implemented by the selected physical link.
```

- [ ] **Step 4: JL firmware hardware logs**

Expected serial milestones:

```text
JL24G init
le_high_rate_init
adv enabled OR create connection
connected
HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE
JL24G ready
open frame_len=<configured>
audio tx count increments
audio rx count increments on peer
HID report tx through HOGP path
disconnect cleanup
```

- [ ] **Step 5: Coexistence test**

Run:

```text
JLA audio stream for 10 minutes
HID report every 1 ms, 2 ms, 4 ms, 8 ms profiles
Disconnect/reconnect every 30 seconds for 20 cycles
```

Pass criteria:

```text
No crash.
No stuck READY state after disconnect.
Audio drop counter is bounded and visible.
HID still reports through HOGP.
Fast RX callback does not run protocol parsing directly.
```

---

### Task 9: Bring-Up Order

**Files:**
- No code files in this task.

- [ ] **Step 1: JL single-side proof**

Use the JL demo as-is and confirm high-rate audio path:

```text
low_latency_tx_node
  -> le_high_rate_transport_instance()->send_data
  -> ble_event_irq_hook
  -> le_hw_send_packet_fast
```

- [ ] **Step 2: Generalplus stub proof**

Use `ll_bearer_stub_backend` to prove the product protocol stack can start, send, receive fake events, and shut down.

- [ ] **Step 3: Host-link proof**

Implement only `jl24g_host_link_tx()` and RX dispatch. Confirm command/event framing works with INIT/START/OPEN/CLOSE before sending audio.

- [ ] **Step 4: JLA one-way proof**

Send fixed-size JLA-like frames from Generalplus to JL:

```text
seq | timestamp | payload
```

Confirm peer receives monotonic `seq`.

- [ ] **Step 5: JLA bidirectional proof**

Enable `HIGH_RATE_UPDOWNSTREAM_MODE` and confirm both sides can send and receive.

- [ ] **Step 6: HID coexistence proof**

Keep HID on `hal_bt_hid_tx()` and confirm JLA streaming does not block HID reports.

- [ ] **Step 7: Recovery proof**

Disconnect, reconnect, reopen, and resume JLA without rebooting either side.

---

## Risks And Decisions

- **Risk:** Copying JL high-rate source into Generalplus directly will not work if the Generalplus CPU does not own the JL controller/baseband.
  - **Decision:** Generalplus side uses command/event adapter; JL side owns actual fast packet calls.
- **Risk:** `BT_BLE_RF` may not be compiled in by default.
  - **Decision:** HID uses `BT_BLE` first; fast audio uses `le_high_rate_transport` wrapper.
- **Risk:** RX callback context may be timing-sensitive.
  - **Decision:** RX callback only copies to a queue and wakes the protocol task.
- **Risk:** Audio and HID have different delivery semantics.
  - **Decision:** Audio may drop oldest frame under pressure; HID remains HOGP/control path.
- **Risk:** ATBM BLE code exists but is disabled by `CONFIG_WIFI_BT_COMB=0`.
  - **Decision:** Do not enable ATBM BLE during JL 2.4G bring-up; treat it as a later backend.

## Done Definition

The integration is done when:

- Generalplus product protocol code calls only `ll_bearer_*` APIs for low-latency link access.
- JL firmware owns `le_high_rate_transport_instance()` and `le_hw_send_packet_fast()`.
- JLA audio packets flow over the JL high-rate path.
- HID reports still flow through HID/HOGP.
- Disconnect/reconnect returns the state machine to READY without reboot.
- Static checks confirm Generalplus host code does not call `le_hw_send_packet_fast()` directly.
- Hardware logs show high-rate open, TX, RX, HID TX, and cleanup counters.

