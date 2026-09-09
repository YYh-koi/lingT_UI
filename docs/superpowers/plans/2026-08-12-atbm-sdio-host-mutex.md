# ATBM SDIO Host Mutex Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 补齐 ATBM6165 Generalplus SDIO Host Mutex 生命周期，使完整同步 SDIO 事务串行化。

**Architecture:** 在 `atbm_gp_sdio_card.c` 封装 Mutex 初始化和销毁；由 `atbm_gp_sdio_init()` / `atbm_gp_sdio_deinit()` 管理生命周期。现有 `sbus_ops->lock()` 和 IRQ 线程继续使用 claim/release 接口，不修改上层协议。

**Tech Stack:** C、CMSIS-RTOS Mutex、FreeRTOS、Generalplus SDIO/DMA。

---

### Task 1: 增加回归检查

**Files:**
- Test: PowerShell 静态断言（不新增运行时代码）

- [ ] **Step 1:** 检查 ATBM 源码当前缺少 `osMutexCreate()`，确认检查失败。
- [ ] **Step 2:** 修改后检查创建、失败清理、反初始化清理以及 IRQ 使能前初始化。

### Task 2: 补齐 Host Mutex 生命周期

**Files:**
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/source/atbm_gp_sdio_card.c`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/include/atbm_gp_sdio_card.h`
- Modify: `gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/sdio/source/atbm_gp_sdio_l2.c`

- [ ] **Step 1:** 新增幂等的 Host Mutex init/deinit 函数。
- [ ] **Step 2:** 在 SDIO 初始化早期创建 Mutex，失败路径释放。
- [ ] **Step 3:** 在关闭 IRQ 和移除设备后释放 Mutex。

### Task 3: 验证

**Files:**
- Verify: 上述三个源文件和 WiFi 库构建目标

- [ ] **Step 1:** 运行静态回归检查，预期全部通过。
- [ ] **Step 2:** 编译 ATBM WiFi 库或受影响构建目标，预期无编译/链接错误。
