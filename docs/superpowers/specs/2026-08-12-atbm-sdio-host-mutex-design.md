# ATBM SDIO Host Mutex Design

## Goal

参照 RTL8189FTV 的 `sdio_claim_host()` 方式，让 ATBM6165 的运行期 SDIO 访问在同一把 Host Mutex 下完成，锁范围覆盖共享数据描述符填写、CMD52/CMD53 发送以及同步 DMA 完成等待。

## Design

- 保留现有 `atbm_gp_sdio_claim_host()` / `atbm_gp_sdio_release_host()` 接口和底层 `sd_sema`。
- 在 ATBM SDIO 初始化路径创建 `atbm_gp_wifi_mutex`，创建失败则中止初始化。
- 在 SDIO 初始化失败和反初始化路径释放 Mutex，避免重复初始化泄漏。
- 不调整 CMD53、DMA、重试和 WSM 逻辑；现有上层 `sbus_ops->lock()` 自动获得有效的 Host Mutex 保护。
- Mutex 必须在使能运行期 SDIO 中断前创建，并在中断关闭、设备移除后销毁。

## Verification

- 静态检查 Mutex 有创建和销毁路径，且初始化调用位于 `vic_irq_enable()` 之前。
- 检查 8189 与 ATBM 的锁范围保持一致。
- 编译 ATBM WiFi 静态库或至少编译受影响源文件，确认声明和链接无误。
