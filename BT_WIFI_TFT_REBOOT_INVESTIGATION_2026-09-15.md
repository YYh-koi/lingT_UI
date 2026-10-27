# 蓝牙 / Wi-Fi / 屏幕启动复位排查记录

日期：2026-09-15  
工程：GPCV2178 Turnkey SDK（ATBM6165、TP2915、LVGL）

## 现象

- 插接屏幕并启动 Wi-Fi/蓝牙时，设备存在偶发整机复位。
- 串口复位后的首段信息为：

  ```text
  io_trap = 0x1D, sys = 72000000, cpu = 216000000
  1st pwr on
  ICE mode ...
  ```

- 物理拔掉屏幕后，多次测试未观察到复位；该结果说明屏幕连接或其相关供电/信号网络会显著影响复现概率。
- 关闭显示任务、背光和 TFT 输出后，屏幕保持插接仍可复位；因此不能归因于 LVGL 刷新、仪表动画、DCLK 或 60 Hz 刷新本身。

## 最新日志定位

第二次复位的关键顺序：

```text
atbm_start_load_firmware: FIRMWARE DOWNLOAD SUCCESS
atbm_after_load_firmware++
atbm_gp_sdio_claim_irq ...
... host IRQ enable suppressed（诊断固件）
io_trap = 0x1D
```

复位发生于 ATBM SDIO IRQ 注册窗口，Wi-Fi 网络栈和 AP 尚未完成启动。该结论仅表明故障时间点与 `claim_irq` 路径相关，尚不能证明是该函数的软件逻辑导致；屏幕连接造成的供电、地弹、共脚或 SDIO 信号完整性问题仍是主要待验证方向。

## 已发现的配置风险

`board_config/inc/board_config.h` 当前把 `IO_F15` 同时定义为：

| 功能 | 宏 |
|---|---|
| 屏幕复位 | `PANEL_IO_RESET` |
| Wi-Fi LDO 使能 | `WIFI_LDO_EN_PIN` |
| 录像状态 LED | `LED` |

录像 LED 原逻辑会周期性翻转该脚，可能同时影响屏幕复位和 Wi-Fi 供电。应用层已保留保护：若 LED 与 Wi-Fi LDO 共脚，则不再由录像 LED 服务写该 GPIO。最终硬件版本应为 LED、屏幕 RESET、Wi-Fi LDO 分配独立网络。

另观察到 `SDC0_POS` 为 `SDC0_MUX7`、`SDIO_WIFI_PINMUX` 为 `SDC0_MUX1`。当前 ATBM 初始化未引用后一个宏，且 SDIO CMD 流程可以成功执行，因此暂不直接修改该配置；硬件原理图应确认 ATBM 实际连接的 SDC0 复用组。

## 已保留的修复

`atbm_os_thread.c` 中原先使用 10 字节局部数组复制 `ELOOP_TASK`、`TX_BH_TASK_`、`atbm_timer` 等任务名，存在确定的栈溢出风险。现改为直接使用常量/传入名称指针；该修复保留。

## 本次恢复项

- 恢复 ATBM 主机 SDIO IRQ 使能。
- 恢复 `atbm_gp_sdio_claim_irq()` 的 IRQ 任务创建和 `CCCR_IEN` 配置。
- 恢复 `C_BT_XSTACK_DEMO_ENABLE = CUSTOM_ON`。
- 恢复 `C_TFT_DISPLAY_FUNC = CUSTOM_ON`，因此 LVGL 条件开关也随之恢复。
- 恢复 `C_DISABLE_SENSOR_TFT = CUSTOM_ON`，以创建 `disp_task`、完成 TP2915 初始化并接收显示消息。

## 后续建议

1. 先以本次恢复固件验证 Wi-Fi、蓝牙与屏幕能够完整启动。
2. 若仍偶发复位，使用示波器同时观察 ATBM 供电、系统 3.3 V、`IO_F15`、SDIO CLK/DAT1，以及屏幕插接和未插接时的差异。
3. 最有价值的硬件隔离实验是在屏幕保持供电连接时，单独断开其 RESET 支路；可区分 `IO_F15` 共脚问题与屏幕电源负载问题。
