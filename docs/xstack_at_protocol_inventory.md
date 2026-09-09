# xStack AT 协议覆盖清单

接口版本：AT Command & Event 260310 V1.2。状态来自当前 xStack 静态代码审计，不代表板端验证结果。

## 命令

| ID | 分组 | 命令 | 中文说明 | xStack 状态 |
|---|---|---|---|---|
| CMD-001 | 通用命令 | `AT` | 通信测试 | 已实现 |
| CMD-002 | 通用命令 | `AT+PROFILE` | 蓝牙 Profile 选择 | 部分实现 |
| CMD-003 | 通用命令 | `AT+VER` | 读取固件版本 | 已实现 |
| CMD-004 | 通用命令 | `AT+BTEN` | 蓝牙电源开关 | 已实现 |
| CMD-005 | 通用命令 | `AT+ADDR` | 读取 BR/EDR 地址 | 已实现 |
| CMD-006 | 通用命令 | `AT+LEADDR` | 读取 BLE 地址 | 待实现 |
| CMD-007 | 通用命令 | `AT+NAME` | 读写 BR/EDR 本地名称 | 已实现 |
| CMD-008 | 通用命令 | `AT+LENAME` | 读写 BLE 本地名称 | 待实现 |
| CMD-009 | 通用命令 | `AT+PIN` | 读写传统配对 PIN 码 | 部分实现 |
| CMD-010 | 通用命令 | `AT+SSP` | 读写配对模式 | 已实现 |
| CMD-011 | 通用命令 | `AT+CFM` | 接受或拒绝配对请求 | 已实现 |
| CMD-012 | 通用命令 | `AT+PAIR` | 设置可发现模式 | 部分实现 |
| CMD-013 | 通用命令 | `AT+PAGE` | 设置 BR/EDR 可连接模式 | 已实现 |
| CMD-014 | 通用命令 | `AT+COD` | 读写设备类别 | 已实现 |
| CMD-015 | 通用命令 | `AT+PLIST` | 读取或清除配对记录 | 部分实现 |
| CMD-016 | 通用命令 | `AT+TDL` | 读写可信设备列表容量策略 | 部分实现 |
| CMD-017 | 通用命令 | `AT+SCAN` | 扫描附近设备 | 部分实现 |
| CMD-018 | 通用命令 | `AT+DSCA` | 释放全部蓝牙连接 | 已实现 |
| CMD-019 | 通用命令 | `AT+REBOOT` | 软件重启 | 待实现 |
| CMD-020 | 通用命令 | `AT+RESTORE` | 恢复出厂设置 | 待实现 |
| CMD-021 | 通用命令 | `AT+MICMUTE` | 麦克风静音控制 | 部分实现 |
| CMD-022 | 通用命令 | `AT+SPKVOL` | 读写音乐音量 | 部分实现 |
| CMD-023 | HFP 命令 | `AT+HFPCFG` | 读写 HFP 配置 | 部分实现 |
| CMD-024 | HFP 命令 | `AT+HFPSTAT` | 读取 HFP 状态 | 已实现 |
| CMD-025 | HFP 命令 | `AT+HFPCONN` | 建立 HFP 连接 | 已实现 |
| CMD-026 | HFP 命令 | `AT+HFPDISC` | 断开 HFP 连接 | 已实现 |
| CMD-027 | HFP 命令 | `AT+HFPDIAL` | 拨号或重拨 | 已实现 |
| CMD-028 | HFP 命令 | `AT+HFPDTMF` | 发送 DTMF | 已实现 |
| CMD-029 | HFP 命令 | `AT+HFPANSW` | 接听来电 | 已实现 |
| CMD-030 | HFP 命令 | `AT+HFPCHUP` | 拒接或挂断 | 已实现 |
| CMD-031 | HFP 命令 | `AT+HFPMCAL` | 三方通话控制 | 已实现 |
| CMD-032 | HFP 命令 | `AT+HFPADTS` | 切换语音音频路由 | 部分实现 |
| CMD-033 | HFP 命令 | `AT+HFPVR` | 远端语音助手控制 | 已实现 |
| CMD-034 | A2DP/AVRCP 命令 | `AT+A2DPSTAT` | 读取 A2DP 状态 | 已实现 |
| CMD-035 | A2DP/AVRCP 命令 | `AT+A2DPCONN` | 建立 A2DP 连接 | 已实现 |
| CMD-036 | A2DP/AVRCP 命令 | `AT+A2DPDISC` | 断开 A2DP 连接 | 已实现 |
| CMD-037 | A2DP/AVRCP 命令 | `AT+A2DPMUTE` | A2DP 静音控制 | 部分实现 |
| CMD-038 | A2DP/AVRCP 命令 | `AT+AVRCPCFG` | 读写 AVRCP 配置 | 部分实现 |
| CMD-039 | A2DP/AVRCP 命令 | `AT+PLAYPAUSE` | 播放/暂停切换 | 已实现 |
| CMD-040 | A2DP/AVRCP 命令 | `AT+PLAY` | 播放 | 已实现 |
| CMD-041 | A2DP/AVRCP 命令 | `AT+PAUSE` | 暂停 | 已实现 |
| CMD-042 | A2DP/AVRCP 命令 | `AT+STOP` | 停止播放 | 已实现 |
| CMD-043 | A2DP/AVRCP 命令 | `AT+NEXT` | 下一曲 | 已实现 |
| CMD-044 | A2DP/AVRCP 命令 | `AT+PREV` | 上一曲 | 已实现 |
| CMD-045 | A2DP/AVRCP 命令 | `AT+FORWARD` | 快进 | 已实现 |
| CMD-046 | A2DP/AVRCP 命令 | `AT+BACKWARD` | 快退 | 已实现 |
| CMD-047 | A2DP/AVRCP 命令 | `AT+REPEAT` | 设置循环模式 | 已实现 |
| CMD-048 | A2DP/AVRCP 命令 | `AT+SHUFFLE` | 设置随机播放 | 已实现 |
| CMD-049 | A2DP/AVRCP 命令 | `AT+GETMP` | 列出远端媒体播放器 | 待实现 |
| CMD-050 | A2DP/AVRCP 命令 | `AT+SETMP` | 选择媒体播放器 | 待实现 |
| CMD-051 | A2DP/AVRCP 命令 | `AT+GETFD` | 列出当前目录内容 | 待实现 |
| CMD-052 | A2DP/AVRCP 命令 | `AT+SETFD` | 进入或返回文件夹 | 待实现 |
| CMD-053 | A2DP/AVRCP 命令 | `AT+GETNP` | 列出正在播放列表 | 待实现 |
| CMD-054 | A2DP/AVRCP 命令 | `AT+ADDMP` | 添加并播放媒体条目 | 待实现 |
| CMD-055 | A2DP/AVRCP 命令 | `AT+GBIPART` | 主动获取专辑封面 | 待实现 |
| CMD-056 | A2DP/AVRCP 命令 | `AT+SABSUP` | 绝对音量增加 | 已实现 |
| CMD-057 | A2DP/AVRCP 命令 | `AT+SABSDOWN` | 绝对音量降低 | 已实现 |
| CMD-058 | PBAP 命令 | `AT+PBDOWN` | 下载电话簿或通话记录 | 待实现 |
| CMD-059 | PBAP 命令 | `AT+PBABORT` | 中止电话簿下载 | 待实现 |
| CMD-060 | PBAP 命令 | `AT+PBCONN` | 主动建立 PBAP 连接 | 待实现 |
| CMD-061 | PBAP 命令 | `AT+PBDISC` | 断开 PBAP 连接 | 待实现 |
| CMD-062 | SPP 命令 | `AT+SPPSTAT` | 读取 SPP 状态 | 待实现 |
| CMD-063 | SPP 命令 | `AT+SPPCONN` | 建立 SPP 连接 | 待实现 |
| CMD-064 | SPP 命令 | `AT+SPPDISC` | 断开 SPP 连接 | 待实现 |
| CMD-065 | SPP 命令 | `AT+SPPSEND` | 通过 SPP 发送数据 | 待实现 |
| CMD-066 | GATT 命令 | `AT+GATTSTAT` | 读取 GATT 状态 | 待实现 |
| CMD-067 | GATT 命令 | `AT+GATTDISC` | 断开 GATT 连接 | 待实现 |
| CMD-068 | GATT 命令 | `AT+GATTSEND` | 通过 GATT 发送数据 | 待实现 |
| CMD-069 | HID 命令 | `AT+HIDCONN` | 建立 HID 连接 | 待实现 |
| CMD-070 | HID 命令 | `AT+HIDDISC` | 断开 HID 连接 | 待实现 |
| CMD-071 | HID 命令 | `AT+HIDKEY` | 发送 HID 遥控按键 | 待实现 |
| CMD-072 | iAP2 映射命令 | `AT+IAPSTAT` | 读取 iAP2 状态 | 待实现 |
| CMD-073 | iAP2 映射命令 | `AT+IAPCONN` | 建立 iAP2 连接 | 待实现 |
| CMD-074 | iAP2 映射命令 | `AT+IAPDISC` | 断开 iAP2 连接 | 待实现 |
| CMD-075 | iAP2 映射命令 | `AT+IAPSEND` | 通过 iAP2 发送数据 | 待实现 |
| CMD-076 | AAP 映射命令 | `AT+AAPSTAT` | 读取 AAP 状态 | 待实现 |
| CMD-077 | AAP 映射命令 | `AT+APPCONN` | 建立 AAP 连接 | 待实现 |
| CMD-078 | AAP 映射命令 | `AT+AAPDISC` | 断开 AAP 连接 | 待实现 |
| CMD-079 | AAP 映射命令 | `AT+APPSEND` | 通过 AAP 发送数据 | 待实现 |

## 事件

| ID | 分组 | 事件 | 中文说明 | xStack 状态 |
|---|---|---|---|---|
| EVT-001 | 通用事件 | `+DEVSTAT` | 设备状态 | 部分实现 |
| EVT-002 | 通用事件 | `+PWRSTAT` | 上下电过程状态 | 已实现 |
| EVT-003 | 通用事件 | `+SCAN` | 扫描结果 | 部分实现 |
| EVT-004 | 通用事件 | `+PAIRREQ` | 配对确认请求 | 已实现 |
| EVT-005 | 通用事件 | `+PAIRED` | 配对结果 | 已实现 |
| EVT-006 | 通用事件 | `+MICMUTED` | 麦克风静音状态 | 部分实现 |
| EVT-007 | HFP 事件 | `+HFPSTAT` | HFP/呼叫状态 | 已实现 |
| EVT-008 | HFP 事件 | `+HFPDEV` | HFP 远端设备信息 | 已实现 |
| EVT-009 | HFP 事件 | `+HFPAUDIO` | HFP 语音音频状态 | 已实现 |
| EVT-010 | HFP 事件 | `+HFPVR` | 语音助手状态 | 已实现 |
| EVT-011 | HFP 事件 | `+HFPSIG` | 远端网络信号强度 | 已实现 |
| EVT-012 | HFP 事件 | `+HFPNET` | 远端网络运营商 | 已实现 |
| EVT-013 | HFP 事件 | `+HFPROAM` | 远端漫游状态 | 已实现 |
| EVT-014 | HFP 事件 | `+HFPBATT` | 远端电池电量 | 已实现 |
| EVT-015 | HFP 事件 | `+HFPMANU` | 远端制造商 | 已实现 |
| EVT-016 | HFP 事件 | `+HFPNUM` | 远端本机号码 | 已实现 |
| EVT-017 | HFP 事件 | `+HFPIBR` | 远端带内铃声能力 | 已实现 |
| EVT-018 | HFP 事件 | `+HFPRING` | 远端来电振铃 | 已实现 |
| EVT-019 | HFP 事件 | `+HFPLOST` | HFP 链路意外丢失 | 已实现 |
| EVT-020 | HFP 事件 | `+HFPMTPX` | 多路呼叫状态 | 已实现 |
| EVT-021 | A2DP/AVRCP 事件 | `+A2DPSTAT` | A2DP 状态 | 已实现 |
| EVT-022 | A2DP/AVRCP 事件 | `+A2DPDEV` | A2DP 远端设备信息 | 已实现 |
| EVT-023 | A2DP/AVRCP 事件 | `+AVRCPSTAT` | AVRCP 状态 | 已实现 |
| EVT-024 | A2DP/AVRCP 事件 | `+A2DPLOST` | A2DP 链路意外丢失 | 已实现 |
| EVT-025 | A2DP/AVRCP 事件 | `+PLAYSTAT` | 播放器状态 | 已实现 |
| EVT-026 | A2DP/AVRCP 事件 | `+PLAYMODE` | 循环/随机播放模式 | 已实现 |
| EVT-027 | A2DP/AVRCP 事件 | `+TRACKSTAT` | 曲目播放进度 | 已实现 |
| EVT-028 | A2DP/AVRCP 事件 | `+TRACKINFO` | 曲目信息 | 已实现 |
| EVT-029 | A2DP/AVRCP 事件 | `+BROWSTAT` | 媒体浏览状态 | 待实现 |
| EVT-030 | A2DP/AVRCP 事件 | `+BROWDATA` | 媒体浏览数据 | 待实现 |
| EVT-031 | A2DP/AVRCP 事件 | `+BIPSTAT` | BIP 封面传输状态 | 待实现 |
| EVT-032 | A2DP/AVRCP 事件 | `+COVERART` | 封面下载成功 | 待实现 |
| EVT-033 | A2DP/AVRCP 事件 | `+ABSVOL` | 绝对音量通知 | 已实现 |
| EVT-034 | PBAP 事件 | `+PBSTAT` | PBAP Client 状态 | 待实现 |
| EVT-035 | PBAP 事件 | `+PBDATA` | 电话簿数据 | 待实现 |
| EVT-036 | SPP 事件 | `+SPPSTAT` | SPP 状态 | 待实现 |
| EVT-037 | SPP 事件 | `+SPPDATA` | SPP 接收数据 | 待实现 |
| EVT-038 | GATT 事件 | `+GATTSTAT` | GATT 状态 | 待实现 |
| EVT-039 | GATT 事件 | `+GATTDATA` | GATT 接收数据 | 待实现 |
| EVT-040 | HID 事件 | `+HIDSTAT` | HID 状态 | 待实现 |
| EVT-041 | ANCS 事件 | `+ANCSSTAT` | ANCS 状态 | 待实现 |
| EVT-042 | ANCS 事件 | `+ANCSNT` | ANCS 通知摘要 | 待实现 |
| EVT-043 | ANCS 事件 | `+ANCSNTI` | iOS 通知详细信息 | 待实现 |
| EVT-044 | ANCS 事件 | `+ANCSNTA` | Android 通知详细信息 | 待实现 |
| EVT-045 | iAP2 映射事件 | `+IAPSTAT` | iAP2 状态 | 待实现 |
| EVT-046 | iAP2 映射事件 | `+IAPDATA` | iAP2 接收数据 | 待实现 |
| EVT-047 | AAP 映射事件 | `+AAPSTAT` | AAP 状态 | 待实现 |
| EVT-048 | AAP 映射事件 | `+AAPDATA` | AAP 接收数据 | 待实现 |
