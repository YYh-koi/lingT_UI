# agents.md — AI 工作约束（GPCV2178 Turnkey SDK）

> 本文件约束 AI 在本工程中的**探索/索引/修改**行为，目的是只读与任务相关的文件，
> 避免全量扫描导致时间和 token 浪费。AI 必须先读完本文件再动手。

## 1. 工程定位（一句话）

凌阳 GPCV2178 SoC 行车记录仪 Turnkey 工程：FreeRTOS + C，
双路视频 + WiFi（RTSP 推流）+ 蓝牙（ATBM6165 xstack，HFP/A2DP/AVRCP）。

## 2. 当前任务上下文

- **目标**：为 **TP2915 RGB888 屏幕**（1280×720，并行 RGB 24bit）开发**蓝牙功能 UI**。
- **UI 方案未最终确认**：先确认 UI 框架（LVGL 是否启用，见 `customer.h` 的 `C_LVGL_SUPPORT`；
  或基于 `application/task_display/` 的 OSD 绘制），再动手。
- 蓝牙能力：`Project/wifi_demo/bt_xstack_demo.c` 已实现 HFP 通话 / A2DP 播放 /
  AVRCP 控制 / 电话本同步的回调与按键处理，UI 要对接这些事件。

## 3. 探索纪律（强制）

1. **禁止全目录扫描**：不得用 LS 递归展开大目录（`gplib/`、`btstack/`、`driver_l1/`、`driver_l2/`、
   `os/`、`prjDebug/`、`.codebase-memory/`）。
2. **先搜索后阅读**：先用 Grep/Glob 定位符号或文件名，再**定点 Read**；一次最多读目标文件，
   禁止"顺手把相邻文件都读了"。
3. **头文件优先**：理解 API 先读 `.h`，`.c` 只在必要时读。
4. **改前必读**：修改任何文件前必须 Read 该文件；修改后检查引用处编译影响。

## 4. 文件白名单（只读这些，按优先级）

### P0 — 必读（蓝牙 UI 开发核心）
| 路径 | 作用 |
|---|---|
| `Project/wifi_demo/bt_xstack_demo.c` / `.h` | 蓝牙演示层：事件回调、状态机、按键处理（UI 对接点） |
| `Project/wifi_demo/bt_audio_task.c` / `.h` | 蓝牙音频任务（A2DP/SCO 音频落 DAC），UI 显示播放状态时关注 |
| `btstack/include/api/bt_xstack.h` 及同目录 `bt_xstack_*.h` | 蓝牙 API（含 `bt_xstack_phone.h`、`bt_xstack_audio.h`、`bt_xstack_phonebook.h`） |
| `Project/GPCV2178_Turnkey_SDK/src/main.c` | 启动流程、任务创建、WiFi/BT 启动入口 |
| `Project/GPCV2178_Turnkey_SDK/inc/customer.h` | 所有功能开关（C_*），改配置先看这里 |
| `Project/Include/project.h` | 全局宏（DBG_PRINT 等）与 FreeRTOS 头引用 |

### P1 — 按需（涉及屏幕/显示时）
- `driver_l2/display/user/tft_tp2915.c`：RGB888 屏驱动（仅读，确认时序/接口）
- `driver_l2/display/src/drv_l2_display.c`、`driver_l2/display/inc/drv_l2_display.h`：显示框架/`DispCtrl_t`
- `board_config/inc/board_config.h`：引脚（TFT1 24bit 数据线、PANEL_IO_*）
- `application/task_display/`（`task_display.c`、`ap_display_func.c/.h`）：显示任务与 OSD 绘制
- `driver_l1/common/inc/drv_l1_tft.h`：TFT 控制器寄存器/数据模式定义（只看宏定义）

### P2 — 只在出现具体问题时读
- `application/inc/application.h`：任务参数/消息宏（`dispTaskQ` 等）
- `Project/GPCV2178_Turnkey_SDK/inc/board_config.h` 的实际位置（见 `board_config/inc/`）

## 5. 文件黑名单（禁止主动读取，除非任务明确涉及，在涉及学习任务时:授权读一切有关蓝牙代码追踪学习的文件，不存在黑名单约束）

| 目录/文件 | 理由 |
|---|---|
| `prjDebug/`（bin/obj/mk/map） | 编译产物，噪音 |
| `.codebase-memory/`、`.task5tmp/`、`*.zip` | 缓存/调试残留 |
| `gplib/liba_wifi_src/**`（atbm 的 hal/net/os、rtk 8189FTV、lwip、wpa） | WiFi 驱动/协议栈源码，与蓝牙 UI 无关 |
| `gplib/fs/`、`gplib/MultiMediaParser/`、`gplib/calendar/` 等 | 通用库，与本任务无关 |
| `os/**`（FreeRTOS 源码） | 系统内核，除非调试任务/内存问题 |
| `driver_l1/**`（除 `drv_l1_tft.h` 宏定义） | 寄存器级驱动，禁止阅读/修改 |
| `btstack/src/`、`btstack/core/`、`btstack/profiles/`、`btstack/at/` 等**实现源码** | 协议栈内部实现，UI 开发只依赖 API；仅当确认协议栈 Bug 才进入 |
| `docs/*.md` | 仅协议文档，按需查 |

> 例外：若某个符号无法在白名单内定位，才允许向外扩展搜索；找到后立即回到白名单。

## 6. 修改纪律

1. **不修改预编译库与协议栈内部**：蓝牙 xstack 以 `btstack/include/api/bt_xstack*.h` 为唯一
   编程接口；`btstack/src/`、`btstack/core/` 等实现不手改（如需修 Bug 先与用户确认）。
2. **功能开关走 `customer.h`**：启用 TP2915 屏 = `LCD_DRV_EN_TP2915 1`（同时关 `LCD_DRV_EN_AUO_A027DTN019`）；
   启用 LVGL = 定义 `LVGL_SUPPORT 1`（会连带 `C_LVGL_SUPPORT`）。
3. **ZXuiIDE/CDK 代码生成会覆盖工程文件**（事件表/回调表），自定义代码要写在不会被覆盖的位置，
   或修改后备份；禁止在生成区塞逻辑。
4. **DRY**：优先复用 `bt_xstack_demo.c` 已有的回调/状态机，不要另起一套。
5. **改动最小化**：只改与任务直接相关的文件；不顺手重构、不加注释到未改动的代码。

## 7. 交付检查（完成前自检）

- [ ] 未读取任何黑名单目录内容

- [ ] 所有修改点都基于已 Read 的文件

- [ ] 屏幕驱动、蓝牙 API、UI 层三处引用关系已核对（谁调用谁）

- [ ] 编译前已确认 `customer.h` 功能开关组合无冲突

- [ ] 需编译检验时，agent不可以私自调用工具编译，这需交给用户来用指定的ide编译验证

  
