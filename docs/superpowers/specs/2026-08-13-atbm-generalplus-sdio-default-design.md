# ATBM Generalplus SDIO 默认配置设计

## 目标

ATBM6165 驱动在未传入 `ATBM_GENERALPLUS_SDIO` 编译宏时，固定按 Generalplus SDIO 平台构建和运行，正式工程不再需要添加 `-DATBM_GENERALPLUS_SDIO=1`。

## 设计

- 将驱动内各处 `ATBM_GENERALPLUS_SDIO` 统一强制定义为 `1`，避免头文件包含顺序或外部编译参数改变 Generalplus 平台选择。
- 从正式主工程和 ATBM WiFi 库工程的编译选项中移除 `-DATBM_GENERALPLUS_SDIO=1`，证明驱动默认配置独立生效。
- 不取消现有宏条件编译结构，但本驱动版本固定用于 Generalplus 平台，不支持通过外部编译参数切换为其他 SDIO 平台。
- 不修改 SDIO 命令、DMA、IRQ、Host Mutex 或 P2P 功能逻辑。

## 验证

- 静态检查所有驱动缺省定义均为 `1`，相关工程不再包含 `-DATBM_GENERALPLUS_SDIO=1`。
- 不传该宏重新编译 ATBM WiFi 库。
- 检查生成的正式 `libatbm_wifi.a`，并记录校验值。
