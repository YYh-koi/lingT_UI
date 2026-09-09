# ATBM PTK/RX Diagnostic Cleanup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 删除 PTK/RX 问题定位专用日志和状态字段，同时保持 WiFi、SDIO 与 AP 行为不变。

**Architecture:** 清理限定在 ATBM common 诊断状态、密钥安装日志、RX 解密/class3 日志和 hostapd 状态推进日志。保留简洁的密钥结果日志及全部 SDIO/AP 诊断；重新构建 WiFi 静态库后由用户链接主工程。

**Tech Stack:** C、ATBM WSM/hostapd、PowerShell、GNU Arm Embedded Toolchain

---

### Task 1: 删除密钥关联诊断状态与日志

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/hal/include/atbm_hal.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/net/atbm_key.c`

- [ ] **Step 1: 记录清理前标记**

Run:

```powershell
rg -n "diag_key_seq|diag_ptk_install|KEY-DIAG" gplib/liba_wifi_src/atbm/atbm6165/hal/include/atbm_hal.h gplib/liba_wifi_src/atbm/atbm6165/net/atbm_key.c
```

Expected: 找到 common 结构体字段和 `atbmwifi_set_key()` 中的三组诊断输出。

- [ ] **Step 2: 删除专用字段和日志块**

从 `struct atbmwifi_common` 删除：

```c
atbm_uint32 diag_key_seq;
atbm_uint32 diag_ptk_install_ms[ATBMWIFI__MAX_STA_IN_AP_MODE + 1];
atbm_uint32 diag_ptk_install_seq[ATBMWIFI__MAX_STA_IN_AP_MODE + 1];
atbm_uint32 diag_rx_decrypt_fail_count;
atbm_uint32 diag_rx_class3_count;
```

从 `atbmwifi_set_key()` 删除 `diag_seq/diag_start_ms/diag_done_ms` 局部变量、`[KEY-DIAG] begin/done` 输出，以及写入 `diag_ptk_install_*` 的代码。保留 `wsm_add_key()` 调用、返回值处理和现有 `[KEY] PTK ... ret=...` 输出。

- [ ] **Step 3: 验证密钥诊断已清理**

Run:

```powershell
rg -n "diag_key_seq|diag_ptk_install|KEY-DIAG" gplib/liba_wifi_src/atbm/atbm6165/hal/include/atbm_hal.h gplib/liba_wifi_src/atbm/atbm6165/net/atbm_key.c
```

Expected: 无匹配，`rg` 返回 1。

### Task 2: 删除 RX 解密与 class3 诊断

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_txrx.c`

- [ ] **Step 1: 删除 class3 诊断计数和输出**

在 `atbmwifi_class3_err()` 中删除 `ATBM_AP_RX_DIAG` 专用局部变量和 `[RX-CLASS3]` 输出块。保留 STA 查找、MAC 比较、Deauth 发送和返回值。

- [ ] **Step 2: 删除解密失败诊断输出**

在 `WSM_STATUS_DECRYPTFAILURE` 分支删除 `[RX-DEC]` 计数、PTK 关联和帧字段输出，保留：

```c
} else if (arg->status == WSM_STATUS_DECRYPTFAILURE) {
    goto drop;
}
```

同时删除仅由该诊断使用的 `frame` 局部变量。

- [ ] **Step 3: 验证 RX 标记已清理**

Run:

```powershell
rg -n "RX-DEC|RX-CLASS3|diag_rx_decrypt_fail_count|diag_rx_class3_count" gplib/liba_wifi_src/atbm/atbm6165
```

Expected: 无匹配，`rg` 返回 1。

### Task 3: 删除 WPA 临时诊断且不恢复误导日志

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/net/wpa/hostapd_main.c`

- [ ] **Step 1: 删除 PTK 安装诊断输出**

删除调用 `wpa_common_install_ptk()` 前的 `[WPA-DIAG] install PTK` 打印，不修改安装调用。

- [ ] **Step 2: 静默正常状态推进**

删除 `[WPA-DIAG] state advance` 输出。仅当状态不是 `PTKINITDONE + KEYINSTALLED` 的正常推进组合时，保留现有 `hostapd retry` 输出；不恢复原来的 `hostapd err Timeout`。

- [ ] **Step 3: 验证四组临时标记全部消失**

Run:

```powershell
rg -n "WPA-DIAG|KEY-DIAG|RX-DEC|RX-CLASS3" gplib/liba_wifi_src/atbm/atbm6165
```

Expected: 无匹配，`rg` 返回 1。

Run:

```powershell
rg -n "SDIO-IRQ|SDIO-RX-RECOVER|APDIAG|\[KEY\] PTK" gplib/liba_wifi_src/atbm/atbm6165
```

Expected: 保留 SDIO/AP 诊断和简洁 PTK 结果日志。

### Task 4: 重新构建 WiFi 静态库

**Files:**
- Generated: `output/wireless_libs/internal/libatbm_wifi.a`
- Generated: `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a`

- [ ] **Step 1: 仅重编 WiFi 源文件并归档**

使用 `tools/wireless_lib/WirelessLibTools.psm1` 生成编译计划，只执行 `WifiActions`，随后调用 `arm-none-eabi-ar/ranlib/strip` 更新两个 `libatbm_wifi.a`。不编译 BTStack。

- [ ] **Step 2: 检查静态库内容**

Run:

```powershell
& 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin\arm-none-eabi-strings.exe' gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a |
    Select-String 'WPA-DIAG|KEY-DIAG|RX-DEC|RX-CLASS3'
```

Expected: 无输出。

- [ ] **Step 3: 交给用户编译主工程**

通知用户在 G+IDE 编译 `GPCV2178_Turnkey_SDK`，使更新后的 `libatbm_wifi.a` 链接进 `project.bin`。当前目录不是 Git 工作区，不执行 commit。
