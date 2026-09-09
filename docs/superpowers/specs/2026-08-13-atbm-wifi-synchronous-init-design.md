# ATBM WiFi 同步初始化接口设计

## 目标

为应用提供明确的 ATBM WiFi 生命周期：先同步初始化公共 WiFi 资源，初始化成功后再启动 AP、STA 或 P2P GO 模式，并消除 P2P 任务异步初始化导致的 GO 启动竞态。

## 公共接口

新增：

```c
int atbm_wifi_init(void);
```

应用调用顺序：

```c
ret = atbm_wifi_init();
if (ret == ATBM_WIFI_OK) {
    ret = atbm_wifi_start_p2p(&config);
}
```

`atbm_wifi_init()` 是同步、幂等接口：

- 首次调用完成 Generalplus SDIO、LwIP、固件、WSM、驱动以及两个虚拟网卡的初始化。
- 只有全部完成才返回 `ATBM_WIFI_OK`。
- 初始化失败时返回现有的 `ATBM_WIFI_ERR_PLATFORM_INIT` 或 `ATBM_WIFI_ERR_DRIVER_INIT`。
- 已初始化时再次调用直接返回 `ATBM_WIFI_OK`，不重复下载固件或创建网卡。

## 模式接口约束

`atbm_wifi_start_ap()`、`atbm_wifi_start_sta()` 和 `atbm_wifi_start_p2p()` 不再隐式执行公共初始化。公共初始化未完成时统一返回：

```c
ATBM_WIFI_ERR_NOT_READY
```

这样应用必须显式调用 `atbm_wifi_init()`，模式接口不会因内部隐式初始化而出现不可见的长耗时或状态变化。

## P2P 同步机制

公共 WiFi 初始化完成不代表 P2P 协议子系统已经完成初始化。P2P 仍在第一次启用时创建 `p2p_main` 任务，因此需要单独同步：

1. `atbm_wifi_start_p2p()` 配置 P2P 参数并调用底层 P2P start。
2. `p2p_main` 任务执行 `atbm_p2p_init()`，包括 WPS/DH 运算和 `priv->p2pdata` 创建。
3. P2P 任务通过完成事件回报成功或失败。
4. `atbm_wifi_start_p2p()` 等待完成事件；成功后才调用 `atbm_wifi_p2p_go_start()`。
5. 初始化失败或等待超时时进行资源回收，并返回 `ATBM_WIFI_ERR_START_FAILED`。

等待使用条件事件，不使用固定延时。超时时间用于防止任务创建失败或异常路径永久阻塞。

## 事件语义

不新增 WiFi 公共初始化完成事件。`atbm_wifi_init()` 返回 `ATBM_WIFI_OK` 本身就是初始化完成通知。

保留现有事件语义：

- `ATBM_WIFI_EVENT_STARTED`：具体 AP、STA 或 P2P GO 模式已经成功启动。
- `ATBM_WIFI_EVENT_ERROR`：模式启动或运行发生错误。

## 应用调整

主工程启动 WiFi 时改为：

1. 调用 `atbm_wifi_init()`。
2. 初始化成功后调用 `atbm_wifi_p2p_mode_enable()`。
3. 初始化失败时打印错误，不启动 P2P 和后续依赖 WiFi 的功能。

## 验证

- 公共初始化接口首次调用成功，重复调用不重复初始化。
- 未调用 `atbm_wifi_init()` 时，三个模式接口都返回 `ATBM_WIFI_ERR_NOT_READY`。
- P2P start 必须等待 P2P 初始化完成，不能在 `priv->p2pdata` 尚未创建时调用 GO start。
- P2P 初始化失败和超时能够退出并清理资源。
- AP、STA 原有启动行为与配置保持不变。
- 重新编译 96 个 WiFi 对象，生成并覆盖正式 `libatbm_wifi.a`。

