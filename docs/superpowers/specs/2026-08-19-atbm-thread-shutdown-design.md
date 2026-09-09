# ATBM FreeRTOS 线程安全退出设计

## 目标

修复 `atbm_wifi_stop_p2p()` 停止 P2P 主任务后，FreeRTOS 任务入口
`atbm_ThreadCallback()` 直接返回并进入 `prvTaskExitError()` 的崩溃/死机问题。

修复保持 P2P 公开 API 和业务停止顺序不变，只明确 ATBM 通用线程
封装中的任务删除所有权。

## 根因

P2P 主任务收到 `NOTHING` 后从 `p2p_main_thread()` 返回。
`atbm_ThreadCallback()` 唤醒 `stopEv` 并打印 `Thread p2p_main stop`，
随后从 FreeRTOS 任务入口返回。ARM CA9 FreeRTOS 端口会进入
`prvTaskExitError()`，关闭中断并永久循环。

现有 `atbm_stopThreadInternal()` 在等到 `stopEv` 后还会对同一任务
调用 `vTaskDelete(handle)`。因此不能只在任务末尾加上自删除，否则会
引入重复删除。

## 所有权模型

正常退出时，运行中的任务负责删除自身；停止调用方只负责请求停止、
等待完成和释放 ATBM 线程包装资源。仅当等待 10 秒超时时，停止
调用方才强制删除任务。

`atbm_ThreadCallback()` 的退出顺序为：

1. 在局部变量中保存 FreeRTOS task handle；
2. 运行完线程回调；
3. 打印线程停止日志；
4. 唤醒 `stopEv`；
5. 使用局部保存的 handle 调用 `atbm_ThreadStopEvent()`，在 FreeRTOS 中
   最终执行 `vTaskDelete(NULL)`。

唤醒 `stopEv` 后，任务不再访问 `atbm_thread_internal_t`，因为停止调用方
可能立即被调度并释放该包装结构。

`atbm_stopThreadInternal()` 的顺序为：

1. 设置 `thread->stop` 并唤醒周期任务；
2. 保存 `atbm_os_wait_event_timeout()` 的返回值；
3. 正常收到停止事件时，不再调用 `atbm_stopThread()`；
4. 超时时才强制调用 `atbm_stopThread()`；
5. 删除等待事件并释放线程包装结构；
6. 正常退出返回 `0`，超时强制删除返回 `-1`。

## 影响范围

修改位于通用 `atbm_thread.c`，因此同样保护 P2P、BH、work queue、timer、
SDIO TX/RX 和 WPA event 等通过 `atbm_createThreadInternal()` 创建的任务。
这些任务统一经由 `atbm_stopThreadInternal()` 停止，不需要改变各自调用方。

P2P 的 GO、WPS、DHCP Server 和 VIF 清理顺序不在本次修复中改变。

## 错误处理

- 正常退出：任务自删除，停止调用返回 `0`。
- 回调未退出：等待 10 秒后强制删除，停止调用返回 `-1`。
- 不改变现有 P2P 公开错误码和 API/ABI。

## 验证

新增聚焦的线程停止契约测试，先观察它因现有任务入口直接返回而
失败，再验证：

1. 正常退出调用 `atbm_ThreadStopEvent()`；
2. 唤醒 `stopEv` 后不再解引用线程包装结构；
3. `atbm_stopThreadInternal()` 仅在等待超时时强制删除任务；
4. FreeRTOS 的 `atbm_ThreadStopEvent()` 仍映射为 `vTaskDelete(NULL)`；
5. P2P 停止仍发送 `NOTHING` 并调用通用停止函数；
6. 现有 ATBM P2P/API 契约通过；
7. 如果本地 ARM 库构建工具可用，重新编译 `ATBM_WiFi_Library`。

## 范围外事项

- 不处理 `wps: cancel, iftype is not support.` 警告；
- 不改变 P2P GO 关闭时的客户端断开顺序；
- 不修改 IP 配置和 DHCP Server 逻辑；
- 不修改公共 `atbm_wifi.h`。
