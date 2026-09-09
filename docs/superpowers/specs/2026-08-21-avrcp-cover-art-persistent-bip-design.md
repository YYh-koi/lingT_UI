# AVRCP Cover Art 持久 BIP 会话设计

## 目标

BIP/OBEX 连接跟随 AVRCP 生命周期。AVRCP 已连接期间保持 BIP 会话；Image Handle 变化时自动在现有 BIP 会话中获取最新封面，不因切歌主动断开并重连。

## 生命周期

- AVRCP Cover Art 可用且 AVRCP 连接后，建立一次 BIP/OBEX 连接。
- 正常完成 GetLinkedThumbnail、GetImageProperties 或 GetImage 后，BIP 保持 `CONNECTED`。
- AVRCP 断开或 Cover Art callback 被关闭时，才主动结束 BIP 会话。
- BIP 因链路或协议错误意外断开时，如果 AVRCP 仍连接且 callback 仍启用，则自动重连。

## Handle 与操作调度

- 保存当前 Handle；收到相同 Handle 时不重复获取。
- BIP 空闲且收到不同 Handle时，立即在现有连接上启动获取。
- 操作进行中收到不同 Handle 时，不调用 cancel，不断开 BIP；仅保存最新 Handle 和 generation。
- 连续收到多个不同 Handle 时采用 latest-wins，只保留最后一个等待项。
- 当前原生操作自然结束后，立即启动最后一个等待 Handle。

## 过期数据

- 切歌后，当前正在传输的 generation 标记为过期。
- 过期 generation 的 DATA、PROPERTIES 和最终成功事件不再转发给上层，避免旧封面覆盖新歌曲。
- 终态仍由内部消费，用于释放操作状态并启动最新等待项。
- 真正的 BIP 错误仍可通知上层；内部在 AVRCP 仍连接时负责恢复 BIP 会话。

## 状态边界

- `connected` 表示 AVRCP 生命周期。
- `obex_ready` 表示 BIP/OBEX 会话可执行请求。
- `operation_active` 表示一个原生 BIP GET 正在执行。
- `active_stale` 表示当前操作结果已因 Handle/track 变化而过期。
- `pending_valid` 保存唯一的最新待处理 Handle。

## 验证

- 一次 AVRCP 连接只建立一次 BIP，连续两个 Handle 使用同一连接完成两次 GET。
- 活动传输中切歌不会调用 cancel；旧 DATA/COMPLETED 不上报，新 Handle 在旧操作结束后自动启动。
- 快速三次 Handle 变化只获取最后一个等待 Handle。
- 相同 Handle 不重复获取。
- AVRCP 断开才触发 BIP peer disconnect。
- BIP 异常断开且 AVRCP 仍连接时自动重连，并在连接恢复后继续最新 Handle。

## 非目标

- 不新增 OBEX Abort。
- 不改变公开 Cover Art API、图片格式、显示流程或 AT/API 构建边界。
