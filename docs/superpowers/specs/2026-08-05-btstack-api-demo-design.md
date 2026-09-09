# BTstack API Demo 改造设计

## 目标

将现有 `Project/wifi_demo/bt_xstack_demo.c` 从 AT 文本接口完全切换到
`btstack_api` 提供的 `bt_xstack_*` 类型化 C API。主应用工程只链接
`libxstack_api.a`，不提供切回旧 `libxstack.a` 的编译开关。

原 `btstack/`、旧 `libxstack.a` 和其中的 AT 实现保持不变；本次只修改应用
Demo、主应用工程配置以及对应契约测试。

## Demo 初始化与启动

`bt_xstack_demo_start()` 继续作为应用唯一入口，执行顺序为：

1. 启动平台音频任务 `bt_audio_task_start()`。
2. 填充 `bt_xstack_config_t`，注册 service、connection、phone、audio 和 PCM
   callback；当前 Demo 不需要处理的 callback 可留空。
3. 调用 `bt_xstack_init()`。
4. 注册通话键和媒体键处理器。
5. 将启动状态置为等待上电事件并调用 `bt_xstack_enable()`。

任一步骤同步返回失败时，Demo 返回对应错误；如果音频任务已经启动，则停止
音频任务。异步控制 API 返回成功仅表示请求已入队，不作为蓝牙状态已经改变的
依据。

## 类型化事件处理

### Service callback

- `BT_XSTACK_SERVICE_EVENT_POWER_STATE` 的 `values[0]` 为 1 时，调用
  `bt_xstack_set_local_name("GP-BT-ATBM6165", false)`。
- 设置名称请求成功入队后，Demo 进入已启动状态；失败则回到等待上电状态。
- 上电状态为 0 时，清除 HFP 响铃、HFP 状态和 AVRCP 连接状态，并保持等待
  下一次上电事件。

### Phone callback

- `BT_XSTACK_PHONE_EVENT_STATE` 直接使用 `values[0]` 更新
  `bt_xstack_hfp_state_t` 状态。
- `BT_XSTACK_PHONE_EVENT_RING` 使用 `values[0]` 更新响铃状态。
- 离开来电相关状态时清除响铃标记。
- `BT_XSTACK_PHONE_EVENT_LOST` 清除响铃状态并恢复 HFP idle。

### Connection callback

- AVRCP profile 进入 `BT_XSTACK_CONNECTION_CONNECTED` 时允许媒体按键处理。
- AVRCP profile 进入其他状态时禁用媒体按键处理。
- HFP/A2DP 的具体业务状态仍分别由 phone/audio callback 处理。

### Audio 与 PCM callback

- `BT_XSTACK_AUDIO_EVENT_PCM_CONFIG` 使用 `values[0..2]` 调用
  `bt_audio_task_configure(sample_rate, channel_count, bits_per_sample)`。
- PCM callback 直接将数据和长度传给 `bt_audio_task_write_pcm()`。
- 不再解析 `+A2DPPCMCONFIG` 等 AT 事件文本。

## 按键映射

- 接听：`bt_xstack_hfp_answer()`。
- 挂断：`bt_xstack_hfp_hang_up()`。
- 上一曲：`bt_xstack_avrcp_previous()`。
- 播放/暂停：`bt_xstack_avrcp_play_pause()`。
- 下一曲：`bt_xstack_avrcp_next()`。

原有按键消费条件保持不变：只有当前 HFP 状态允许时才消费通话键；只有 AVRCP
已连接时才消费媒体键。API 请求因队列满或参数状态被拒绝时仍记录状态码，但按键
分发行为保持与现有 Demo 一致。

## 主工程配置

修改 `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`
中所有相关构建目标：

- 公共 API include 路径使用 `../../btstack_api/include/api`。
- 为 `bt_audio_task.c` 保留必要的 `btstack_api/ports/gpcv2178` 配置头路径。
- 移除旧 `btstack/at`、`btstack/include/adapter` 等 AT/manager 专用 include 路径。
- 链接库由 `gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a` 切换为
  `gplib/liba_wifi_src/atbm/atbm6165/bt_api/libxstack_api.a`。
- Demo 源文件与公共 Demo 头文件继续使用现有工程单元，不复制第二份 Demo。

## 测试与验收

先修改 `Project/wifi_demo/tests/test_bt_xstack_demo_contract.ps1`，使其在旧实现上
明确失败，然后再修改生产代码。契约至少验证：

- Demo 不包含 AT 命令字符串、AT 事件前缀、`sscanf()` 或
  `bt_xstack_send_at()`。
- Demo 使用 `bt_xstack_config_t` 和 service/connection/phone/audio/PCM callbacks。
- 初始化、上电、设置名称、通话按键、媒体按键和 PCM 配置均调用对应的
  `bt_xstack_*` API。
- 主工程不链接 `libxstack.a`，只链接 `libxstack_api.a`。
- 主工程不再包含旧 AT/manager include 路径。

通过契约后，重新构建 `libxstack_api.a`，再生成主应用 Makefile并执行主应用编译。
若完整主应用受仓库既有环境问题阻塞，需要保留契约结果和具体构建错误；目标板上的
上电、配对、HFP、A2DP/AVRCP 和 PCM 播放仍属于实机验证范围。
