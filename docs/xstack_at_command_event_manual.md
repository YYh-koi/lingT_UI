# xStack AT 命令与事件完整协议手册

版本：V1.2  
日期：2026-08-03  
接口版本：AT Command & Event 260310 V1.2

## 1 Command 命令总表

| ID | Command | 中文功能简述 | 状态 |
|---|---|---|---|
| CMD-001 | AT | 通信测试 | 已实现 |
| CMD-002 | AT+PROFILE | 蓝牙 Profile 选择 | 部分实现 |
| CMD-003 | AT+VER | 读取固件版本 | 已实现 |
| CMD-004 | AT+BTEN | 蓝牙电源开关 | 已实现 |
| CMD-005 | AT+ADDR | 读取 BR/EDR 地址 | 已实现 |
| CMD-006 | AT+LEADDR | 读取 BLE 地址 | 待实现 |
| CMD-007 | AT+NAME | 读写 BR/EDR 本地名称 | 已实现 |
| CMD-008 | AT+LENAME | 读写 BLE 本地名称 | 待实现 |
| CMD-009 | AT+PIN | 读写传统配对 PIN 码 | 部分实现 |
| CMD-010 | AT+SSP | 读写配对模式 | 已实现 |
| CMD-011 | AT+CFM | 接受或拒绝配对请求 | 已实现 |
| CMD-012 | AT+PAIR | 设置可发现模式 | 部分实现 |
| CMD-013 | AT+PAGE | 设置 BR/EDR 可连接模式 | 已实现 |
| CMD-014 | AT+COD | 读写设备类别 | 已实现 |
| CMD-015 | AT+PLIST | 读取或清除配对记录 | 部分实现 |
| CMD-016 | AT+TDL | 读写可信设备列表容量策略 | 部分实现 |
| CMD-017 | AT+SCAN | 扫描附近设备 | 部分实现 |
| CMD-018 | AT+DSCA | 释放全部蓝牙连接 | 已实现 |
| CMD-019 | AT+REBOOT | 软件重启 | 待实现 |
| CMD-020 | AT+RESTORE | 恢复出厂设置 | 待实现 |
| CMD-021 | AT+MICMUTE | 麦克风静音控制 | 部分实现 |
| CMD-022 | AT+SPKVOL | 读写音乐音量 | 部分实现 |
| CMD-023 | AT+HFPCFG | 读写 HFP 配置 | 部分实现 |
| CMD-024 | AT+HFPSTAT | 读取 HFP 状态 | 已实现 |
| CMD-025 | AT+HFPCONN | 建立 HFP 连接 | 已实现 |
| CMD-026 | AT+HFPDISC | 断开 HFP 连接 | 已实现 |
| CMD-027 | AT+HFPDIAL | 拨号或重拨 | 已实现 |
| CMD-028 | AT+HFPDTMF | 发送 DTMF | 已实现 |
| CMD-029 | AT+HFPANSW | 接听来电 | 已实现 |
| CMD-030 | AT+HFPCHUP | 拒接或挂断 | 已实现 |
| CMD-031 | AT+HFPMCAL | 三方通话控制 | 已实现 |
| CMD-032 | AT+HFPADTS | 切换语音音频路由 | 部分实现 |
| CMD-033 | AT+HFPVR | 远端语音助手控制 | 已实现 |
| CMD-034 | AT+A2DPSTAT | 读取 A2DP 状态 | 已实现 |
| CMD-035 | AT+A2DPCONN | 建立 A2DP 连接 | 已实现 |
| CMD-036 | AT+A2DPDISC | 断开 A2DP 连接 | 已实现 |
| CMD-037 | AT+A2DPMUTE | A2DP 静音控制 | 部分实现 |
| CMD-038 | AT+AVRCPCFG | 读写 AVRCP 配置 | 部分实现 |
| CMD-039 | AT+PLAYPAUSE | 播放/暂停切换 | 已实现 |
| CMD-040 | AT+PLAY | 播放 | 已实现 |
| CMD-041 | AT+PAUSE | 暂停 | 已实现 |
| CMD-042 | AT+STOP | 停止播放 | 已实现 |
| CMD-043 | AT+NEXT | 下一曲 | 已实现 |
| CMD-044 | AT+PREV | 上一曲 | 已实现 |
| CMD-045 | AT+FORWARD | 快进 | 已实现 |
| CMD-046 | AT+BACKWARD | 快退 | 已实现 |
| CMD-047 | AT+REPEAT | 设置循环模式 | 已实现 |
| CMD-048 | AT+SHUFFLE | 设置随机播放 | 已实现 |
| CMD-049 | AT+GETMP | 列出远端媒体播放器 | 待实现 |
| CMD-050 | AT+SETMP | 选择媒体播放器 | 待实现 |
| CMD-051 | AT+GETFD | 列出当前目录内容 | 待实现 |
| CMD-052 | AT+SETFD | 进入或返回文件夹 | 待实现 |
| CMD-053 | AT+GETNP | 列出正在播放列表 | 待实现 |
| CMD-054 | AT+ADDMP | 添加并播放媒体条目 | 待实现 |
| CMD-055 | AT+GBIPART | 主动获取专辑封面 | 待实现 |
| CMD-056 | AT+SABSUP | 绝对音量增加 | 已实现 |
| CMD-057 | AT+SABSDOWN | 绝对音量降低 | 已实现 |
| CMD-058 | AT+PBDOWN | 下载电话簿或通话记录 | 待实现 |
| CMD-059 | AT+PBABORT | 中止电话簿下载 | 待实现 |
| CMD-060 | AT+PBCONN | 主动建立 PBAP 连接 | 待实现 |
| CMD-061 | AT+PBDISC | 断开 PBAP 连接 | 待实现 |
| CMD-062 | AT+SPPSTAT | 读取 SPP 状态 | 待实现 |
| CMD-063 | AT+SPPCONN | 建立 SPP 连接 | 待实现 |
| CMD-064 | AT+SPPDISC | 断开 SPP 连接 | 待实现 |
| CMD-065 | AT+SPPSEND | 通过 SPP 发送数据 | 待实现 |
| CMD-066 | AT+GATTSTAT | 读取 GATT 状态 | 待实现 |
| CMD-067 | AT+GATTDISC | 断开 GATT 连接 | 待实现 |
| CMD-068 | AT+GATTSEND | 通过 GATT 发送数据 | 待实现 |
| CMD-069 | AT+HIDCONN | 建立 HID 连接 | 待实现 |
| CMD-070 | AT+HIDDISC | 断开 HID 连接 | 待实现 |
| CMD-071 | AT+HIDKEY | 发送 HID 遥控按键 | 待实现 |
| CMD-072 | AT+IAPSTAT | 读取 iAP2 状态 | 待实现 |
| CMD-073 | AT+IAPCONN | 建立 iAP2 连接 | 待实现 |
| CMD-074 | AT+IAPDISC | 断开 iAP2 连接 | 待实现 |
| CMD-075 | AT+IAPSEND | 通过 iAP2 发送数据 | 待实现 |
| CMD-076 | AT+AAPSTAT | 读取 AAP 状态 | 待实现 |
| CMD-077 | AT+APPCONN | 建立 AAP 连接 | 待实现 |
| CMD-078 | AT+AAPDISC | 断开 AAP 连接 | 待实现 |
| CMD-079 | AT+APPSEND | 通过 AAP 发送数据 | 待实现 |

## 2 Event 事件总表

| ID | Event | 中文功能简述 | 状态 |
|---|---|---|---|
| EVT-001 | +DEVSTAT | 设备状态 | 部分实现 |
| EVT-002 | +PWRSTAT | 上下电过程状态 | 已实现 |
| EVT-003 | +SCAN | 扫描结果 | 部分实现 |
| EVT-004 | +PAIRREQ | 配对确认请求 | 已实现 |
| EVT-005 | +PAIRED | 配对结果 | 已实现 |
| EVT-006 | +MICMUTED | 麦克风静音状态 | 部分实现 |
| EVT-007 | +HFPSTAT | HFP/呼叫状态 | 已实现 |
| EVT-008 | +HFPDEV | HFP 远端设备信息 | 已实现 |
| EVT-009 | +HFPAUDIO | HFP 语音音频状态 | 已实现 |
| EVT-010 | +HFPVR | 语音助手状态 | 已实现 |
| EVT-011 | +HFPSIG | 远端网络信号强度 | 已实现 |
| EVT-012 | +HFPNET | 远端网络运营商 | 已实现 |
| EVT-013 | +HFPROAM | 远端漫游状态 | 已实现 |
| EVT-014 | +HFPBATT | 远端电池电量 | 已实现 |
| EVT-015 | +HFPMANU | 远端制造商 | 已实现 |
| EVT-016 | +HFPNUM | 远端本机号码 | 已实现 |
| EVT-017 | +HFPIBR | 远端带内铃声能力 | 已实现 |
| EVT-018 | +HFPRING | 远端来电振铃 | 已实现 |
| EVT-019 | +HFPLOST | HFP 链路意外丢失 | 已实现 |
| EVT-020 | +HFPMTPX | 多路呼叫状态 | 已实现 |
| EVT-021 | +A2DPSTAT | A2DP 状态 | 已实现 |
| EVT-022 | +A2DPDEV | A2DP 远端设备信息 | 已实现 |
| EVT-023 | +AVRCPSTAT | AVRCP 状态 | 已实现 |
| EVT-024 | +A2DPLOST | A2DP 链路意外丢失 | 已实现 |
| EVT-025 | +PLAYSTAT | 播放器状态 | 已实现 |
| EVT-026 | +PLAYMODE | 循环/随机播放模式 | 已实现 |
| EVT-027 | +TRACKSTAT | 曲目播放进度 | 已实现 |
| EVT-028 | +TRACKINFO | 曲目信息 | 已实现 |
| EVT-029 | +BROWSTAT | 媒体浏览状态 | 待实现 |
| EVT-030 | +BROWDATA | 媒体浏览数据 | 待实现 |
| EVT-031 | +BIPSTAT | BIP 封面传输状态 | 待实现 |
| EVT-032 | +COVERART | 封面下载成功 | 待实现 |
| EVT-033 | +ABSVOL | 绝对音量通知 | 已实现 |
| EVT-034 | +PBSTAT | PBAP Client 状态 | 待实现 |
| EVT-035 | +PBDATA | 电话簿数据 | 待实现 |
| EVT-036 | +SPPSTAT | SPP 状态 | 待实现 |
| EVT-037 | +SPPDATA | SPP 接收数据 | 待实现 |
| EVT-038 | +GATTSTAT | GATT 状态 | 待实现 |
| EVT-039 | +GATTDATA | GATT 接收数据 | 待实现 |
| EVT-040 | +HIDSTAT | HID 状态 | 待实现 |
| EVT-041 | +ANCSSTAT | ANCS 状态 | 待实现 |
| EVT-042 | +ANCSNT | ANCS 通知摘要 | 待实现 |
| EVT-043 | +ANCSNTI | iOS 通知详细信息 | 待实现 |
| EVT-044 | +ANCSNTA | Android 通知详细信息 | 待实现 |
| EVT-045 | +IAPSTAT | iAP2 状态 | 待实现 |
| EVT-046 | +IAPDATA | iAP2 接收数据 | 待实现 |
| EVT-047 | +AAPSTAT | AAP 状态 | 待实现 |
| EVT-048 | +AAPDATA | AAP 接收数据 | 待实现 |

## 3 通用协议格式与状态说明

### 3.1 文档说明

本手册完整收录 xStack AT 接口规划中的 79 条命令和 48 类事件，并包含 iAP2/AAP 映射项。中文说明用于客户集成、AT 联调和后续功能开发。支持状态来自静态代码审计；待实现条目用于描述后续接口目标，不表示当前库已经具备该能力。

### 3.2 通用协议格式

- 命令：`AT+<command>[=<parameter>...]\r\n`。独立 `AT` 用于通信测试。
- 成功：查询通常先返回 `+<name>=...`，再返回 `OK\r\n`；设置或执行成功返回 `OK\r\n`。
- 失败：`ERROR\r\n`。
- 事件：`+<event>[=<parameter>...]\r\n`。
- `<FF>`：ASCII Form Feed 字节 `0x0C`，不是字符串 `<FF>`，也不是字节 `0xFF`。

### 3.3 支持状态

状态定义：**已实现**表示当前存在解析和后端或格式化路径；**部分实现**表示参数、模式、数据、音频路径或持久化尚未完全对齐；**待实现**表示 AT 路径或底层 Profile 尚不可用。

- Command 共 79 条：已实现 34，部分实现 12，待实现 33。
- Event 共 48 类：已实现 26，部分实现 3，待实现 19。

## 4 Command 详细说明

### 通用命令

#### CMD-001 AT - 通信测试

**支持状态：已实现**

**格式：** `AT`

**参数：** 无。

**响应：** OK

**说明：** 用于确认 HOST 与 xStack AT 接口之间的命令通道是否正常。上电、切换传输通道或调整串口参数后可优先发送本命令。

```text
HOST> AT
xStack> OK
```

#### CMD-002 AT+PROFILE - 蓝牙 Profile 选择

**支持状态：部分实现**

**格式：** `AT+PROFILE[=<Param>]`

**参数：** Param 为十进制位掩码：BIT0 SPP；BIT1 GATT Server；BIT2 GATT Client；BIT3 HFP Sink；BIT4 HFP Source；BIT5 A2DP Sink；BIT6 A2DP Source；BIT7 AVRCP Controller；BIT8 AVRCP Target；BIT9 HID Keyboard；BIT10 PBAP Server；BIT14 AAP；BIT15 iAP2。缺省值为 1192。

**响应：** 查询：+PROFILE=<Param> 后跟 OK；设置成功：OK。

**说明：** 读取或设置启用的蓝牙 Profile。互斥角色不能同时启用。当前 xStack 仅支持固定掩码 424（HFP Sink、A2DP Sink、AVRCP Controller、AVRCP Target），其余 Profile 位待实现。

```text
HOST> AT+PROFILE
xStack> +PROFILE=424
xStack> OK
```

#### CMD-003 AT+VER - 读取固件版本

**支持状态：已实现**

**格式：** `AT+VER`

**参数：** 无。

**响应：** +VER=<版本字符串> 后跟 OK。

**说明：** 读取当前 xStack 固件或静态库的版本标识。

```text
HOST> AT+VER
xStack> +VER=GPCV2178_BTSTACK_1.0
xStack> OK
```

#### CMD-004 AT+BTEN - 蓝牙电源开关

**支持状态：已实现**

**格式：** `AT+BTEN=<Param>`

**参数：** Param：0 关闭蓝牙；1 开启蓝牙。

**响应：** 设置成功：OK。电源状态通过 +PWRSTAT 和 +DEVSTAT 异步上报。

**说明：** 控制蓝牙控制器和协议栈上电或下电。模块上电后默认自动启动蓝牙；切换期间不建议连续发送其他命令。

```text
HOST> AT+BTEN=1
xStack> OK
xStack> +PWRSTAT=1
```

#### CMD-005 AT+ADDR - 读取 BR/EDR 地址

**支持状态：已实现**

**格式：** `AT+ADDR`

**参数：** 无。

**响应：** +ADDR=<12 位十六进制地址> 后跟 OK。

**说明：** 读取本机 BR/EDR MAC 地址，输出不含冒号并使用大写十六进制。

```text
HOST> AT+ADDR
xStack> +ADDR=DC0D30123456
xStack> OK
```

#### CMD-006 AT+LEADDR - 读取 BLE 地址

**支持状态：待实现**

**格式：** `AT+LEADDR`

**参数：** 无。

**响应：** +LEADDR=<12 位十六进制地址> 后跟 OK。

**说明：** 读取本机 BLE MAC 地址。当前 xStack AT 层尚未提供此命令。

```text
HOST> AT+LEADDR
预期> +LEADDR=DC0D30123456
预期> OK
```

#### CMD-007 AT+NAME - 读写 BR/EDR 本地名称

**支持状态：已实现**

**格式：** `AT+NAME[=<Param1>[,<Param2>]]`

**参数：** Param1：1~31 字节 ASCII 名称，缺省名称为 bluetooth_auto。Param2：0 不追加地址后缀；1 追加 -XXXX（MAC 地址末四位），缺省值为 0。

**响应：** 查询：+NAME=<名称> 后跟 OK；设置成功：OK。

**说明：** 未带参数时读取当前名称，带参数时设置名称。当前 xStack 配置保存在 RAM，重启后恢复默认值。

```text
HOST> AT+NAME=ABC,1
xStack> OK
```

#### CMD-008 AT+LENAME - 读写 BLE 本地名称

**支持状态：待实现**

**格式：** `AT+LENAME[=<Param1>[,<Param2>]]`

**参数：** Param1：1~25 字节 ASCII 名称，默认 bluetooth_auto。Param2：0 不追加地址后缀；1 追加 -XXXX，默认 0。

**响应：** 查询：+LENAME=<名称> 后跟 OK；设置成功：OK。

**说明：** 读取或设置 BLE 广播名称。当前 xStack AT 层尚未提供此命令。

```text
HOST> AT+LENAME=ABC,1
预期> OK
```

#### CMD-009 AT+PIN - 读写传统配对 PIN 码

**支持状态：部分实现**

**格式：** `AT+PIN[=<Param>]`

**参数：** Param：4~15 字节 ASCII PIN，默认 0000。

**响应：** 查询：+PIN=<PIN> 后跟 OK；设置成功：OK。

**说明：** 设置传统配对模式使用的 PIN，仅在 AT+SSP=0 时生效。当前 xStack 将配置保存在 RAM。

```text
HOST> AT+PIN=12345678
xStack> OK
```

#### CMD-010 AT+SSP - 读写配对模式

**支持状态：已实现**

**格式：** `AT+SSP[=<Param>]`

**参数：** Param：0 传统 PIN 配对；1 SSP 自动配对；2 SSP 显示确认；3 SSP 数字比较并由 AT+CFM 接受或拒绝。默认 2。

**响应：** 查询：+SSP=<Param> 后跟 OK；设置成功：OK。

**说明：** 选择 BR/EDR 安全简单配对模式。模式 3 收到 +PAIRREQ 后应使用 AT+CFM 响应。

```text
HOST> AT+SSP=3
xStack> OK
```

#### CMD-011 AT+CFM - 接受或拒绝配对请求

**支持状态：已实现**

**格式：** `AT+CFM=<Param1>,<Param2>`

**参数：** Param1：远端设备 12 位 MAC 地址。Param2：0 拒绝；1 接受。

**响应：** 处理成功：OK；地址或状态不匹配：ERROR。

**说明：** 仅用于 SSP 模式 3。必须存在来自同一地址的待确认请求。

```text
HOST> AT+CFM=1C5CF226D773,1
xStack> OK
```

#### CMD-012 AT+PAIR - 设置可发现模式

**支持状态：部分实现**

**格式：** `AT+PAIR[=<Param>]`

**参数：** Param：0 退出 BR/EDR/BLE 可发现；1 仅 BR/EDR 可发现；2 仅 BLE 广播；3 同时 BR/EDR 可发现和 BLE 广播。

**响应：** 设置成功：OK。

**说明：** 控制扫描响应/广播可见性。当前 xStack 仅支持 BR/EDR 模式 0、1；模式 2、3 待实现。无参数等价于 AT+PAIR=1。

```text
HOST> AT+PAIR=1
xStack> OK
```

#### CMD-013 AT+PAGE - 设置 BR/EDR 可连接模式

**支持状态：已实现**

**格式：** `AT+PAGE[=<Param>]`

**参数：** Param：0 退出可连接模式；1 进入可连接模式。

**响应：** 设置成功：OK。

**说明：** 控制 BR/EDR Page Scan。无参数等价于 AT+PAGE=1。

```text
HOST> AT+PAGE=1
xStack> OK
```

#### CMD-014 AT+COD - 读写设备类别

**支持状态：已实现**

**格式：** `AT+COD[=<Param>]`

**参数：** Param：6 位十六进制 Class of Device，缺省值为 240404。

**响应：** 查询：+COD=<6 位十六进制值> 后跟 OK；设置成功：OK。

**说明：** 读取或设置 BR/EDR Class of Device。

```text
HOST> AT+COD
xStack> +COD=240404
xStack> OK
```

#### CMD-015 AT+PLIST - 读取或清除配对记录

**支持状态：部分实现**

**格式：** `AT+PLIST[=<Param>]`

**参数：** Param：0 清除全部；1~8 按索引清除；12 位 MAC 按地址清除。

**响应：** 查询时逐条返回 +PLIST=<索引><FF><Profile掩码><FF><MAC>[<FF><名称>]，以 +PLIST=E 结束，再返回 OK。清除成功返回 OK。

**说明：** 管理最多 8 条可信设备记录。<FF> 表示字节 0x0C。当前 xStack 使用 RAM 列表，掉电不保存。

```text
HOST> AT+PLIST
xStack> +PLIST=1<FF>424<FF>1C5CF226D773<FF>Phone
xStack> +PLIST=E
xStack> OK
```

#### CMD-016 AT+TDL - 读写可信设备列表容量策略

**支持状态：部分实现**

**格式：** `AT+TDL[=<Param>]`

**参数：** Param：0 自动模式，列表满时替换最旧记录；1~8 限制模式，达到数量后不再进入可发现模式。默认 0。

**响应：** 查询：+TDL=<Param> 后跟 OK；设置成功：OK。

**说明：** 配置可信设备列表容量策略。当前 xStack 的记录只保存在 RAM。

```text
HOST> AT+TDL=8
xStack> OK
```

#### CMD-017 AT+SCAN - 扫描附近设备

**支持状态：部分实现**

**格式：** `AT+SCAN=<Param1>[,<Param2>[,<Param3>]]`

**参数：** Param1：0 停止；1 扫描 BR/EDR；2 扫描 BLE；3 同时扫描。Param2：1~48，扫描时长单位 1.28 秒，默认 10（12.8 秒）。Param3：1~25 字节名称过滤器。

**响应：** 命令接受返回 OK；结果通过 +SCAN 异步上报，结束返回 +SCAN=E。

**说明：** 当前 xStack 仅支持 BR/EDR 模式 0、1；BLE 模式 2、3 待实现。

```text
HOST> AT+SCAN=1,10,Phone
xStack> OK
xStack> +SCAN=1<FF>-42<FF>0<FF>001122334455<FF>Phone<FF>240404
xStack> +SCAN=E
```

#### CMD-018 AT+DSCA - 释放全部蓝牙连接

**支持状态：已实现**

**格式：** `AT+DSCA`

**参数：** 无。

**响应：** 本地操作受理：OK。

**说明：** 请求释放当前所有蓝牙 Profile 连接。最终状态通过各 Profile 状态事件上报。

```text
HOST> AT+DSCA
xStack> OK
```

#### CMD-019 AT+REBOOT - 软件重启

**支持状态：待实现**

**格式：** `AT+REBOOT`

**参数：** 无。

**响应：** 受理后返回 OK，然后断开连接并重启。

**说明：** 释放全部蓝牙连接后执行软件重启。当前 xStack AT 组件尚未提供系统任务重建或平台复位路径。

```text
HOST> AT+REBOOT
预期> OK
```

#### CMD-020 AT+RESTORE - 恢复出厂设置

**支持状态：待实现**

**格式：** `AT+RESTORE`

**参数：** 无。

**响应：** 受理后返回 OK，然后恢复默认配置并重启。

**说明：** 清除持久化配置后恢复缺省设置并重启。当前 xStack 配置主要存储于 RAM，尚未提供对应的持久化恢复流程。

```text
HOST> AT+RESTORE
预期> OK
```

#### CMD-021 AT+MICMUTE - 麦克风静音控制

**支持状态：部分实现**

**格式：** `AT+MICMUTE=<Param>`

**参数：** Param：0 取消静音；1 静音。默认 0。

**响应：** 设置成功：OK，并上报 +MICMUTED=<Param>。

**说明：** 仅在通话活动期间使用。当前 xStack 可维护控制状态，但完整 SCO 麦克风音频处理仍需板端音频路径配合。

```text
HOST> AT+MICMUTE=1
xStack> OK
xStack> +MICMUTED=1
```

#### CMD-022 AT+SPKVOL - 读写音乐音量

**支持状态：部分实现**

**格式：** `AT+SPKVOL[=<Param>]`

**参数：** Param：0~15，默认 14。

**响应：** 查询：+SPKVOL=<Param> 后跟 OK；设置成功：OK。

**说明：** 设置 A2DP 音乐播放音量等级。实际音量生效还依赖板端音频输出和远端绝对音量同步。

```text
HOST> AT+SPKVOL=12
xStack> OK
```

### HFP 命令

#### CMD-023 AT+HFPCFG - 读写 HFP 配置

**支持状态：部分实现**

**格式：** `AT+HFPCFG[=<Param>]`

**参数：** Param 为十进制位掩码：BIT0 上电自动回连；BIT1 来电自动接听；BIT2 三方通话。默认 3。

**响应：** 查询：+HFPCFG=<Param> 后跟 OK；设置成功：OK。

**说明：** 配置 HFP 行为。当前 xStack 存在配置和命令路径，自动接听、三方通话等仍取决于远端 AG 能力和板端验证。

```text
HOST> AT+HFPCFG=3
xStack> OK
```

#### CMD-024 AT+HFPSTAT - 读取 HFP 状态

**支持状态：已实现**

**格式：** `AT+HFPSTAT`

**参数：** 无。

**响应：** +HFPSTAT=<0~10> 后跟 OK。

**说明：** 状态：0 不支持；1 待机；2 连接中；3 已连接；4 去电；5 来电；6 通话中；7 活动呼叫保持；8 第一通活动、第二通等待；9 第一通活动、第二通保持；10 第一通呼出、第二通保持。

```text
HOST> AT+HFPSTAT
xStack> +HFPSTAT=3
xStack> OK
```

#### CMD-025 AT+HFPCONN - 建立 HFP 连接

**支持状态：已实现**

**格式：** `AT+HFPCONN[=<Param>]`

**参数：** Param：目标设备 12 位 MAC 地址；省略时回连最近一次 HFP 设备。

**响应：** 本地连接请求受理：OK；无历史设备或立即失败：ERROR。

**说明：** 发起 HFP Hands-Free 连接。OK 只代表本地请求被接受，最终连接结果通过 +HFPSTAT/+HFPDEV 上报。

```text
HOST> AT+HFPCONN=1C5CF226D773
xStack> OK
```

#### CMD-026 AT+HFPDISC - 断开 HFP 连接

**支持状态：已实现**

**格式：** `AT+HFPDISC`

**参数：** 无。

**响应：** 本地断开请求受理：OK。

**说明：** 释放当前 HFP 连接。

```text
HOST> AT+HFPDISC
xStack> OK
```

#### CMD-027 AT+HFPDIAL - 拨号或重拨

**支持状态：已实现**

**格式：** `AT+HFPDIAL[=<Param>]`

**参数：** Param：1~25 字节电话号码；省略时执行重拨。

**响应：** 命令受理：OK。

**说明：** 要求 HFP 已连接。呼叫状态通过 +HFPSTAT/+HFPMTPX 上报。

```text
HOST> AT+HFPDIAL=10086
xStack> OK
```

#### CMD-028 AT+HFPDTMF - 发送 DTMF

**支持状态：已实现**

**格式：** `AT+HFPDTMF=<Param>`

**参数：** Param：0~9、# 或 * 中的单个字符。

**响应：** 命令受理：OK。

**说明：** 在活动通话期间向远端电话发送 DTMF 音。

```text
HOST> AT+HFPDTMF=#
xStack> OK
```

#### CMD-029 AT+HFPANSW - 接听来电

**支持状态：已实现**

**格式：** `AT+HFPANSW`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 接听当前 HFP 来电。

```text
HOST> AT+HFPANSW
xStack> OK
```

#### CMD-030 AT+HFPCHUP - 拒接或挂断

**支持状态：已实现**

**格式：** `AT+HFPCHUP`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 来电时拒接，去电或通话中执行挂断。

```text
HOST> AT+HFPCHUP
xStack> OK
```

#### CMD-031 AT+HFPMCAL - 三方通话控制

**支持状态：已实现**

**格式：** `AT+HFPMCAL=<Param>`

**参数：** Param：0 释放保持呼叫或拒绝等待呼叫；1 释放当前活动呼叫并接受另一呼叫；2 保持当前活动呼叫并接受另一呼叫。

**响应：** 命令受理：OK。

**说明：** 用于 HFP 三方通话，需远端 AG 支持相应 CHLD 能力。

```text
HOST> AT+HFPMCAL=2
xStack> OK
```

#### CMD-032 AT+HFPADTS - 切换语音音频路由

**支持状态：部分实现**

**格式：** `AT+HFPADTS[=<Param>]`

**参数：** Param：1 将语音从模块转移到远端设备；2 将语音从远端设备转移到模块；省略时在两端之间切换。

**响应：** 命令受理：OK。

**说明：** 控制 SCO 音频连接方向。当前 xStack 可控制连接状态，但完整麦克风/扬声器 PCM 路由需要板端音频实现。

```text
HOST> AT+HFPADTS=2
xStack> OK
```

#### CMD-033 AT+HFPVR - 远端语音助手控制

**支持状态：已实现**

**格式：** `AT+HFPVR=<Param>`

**参数：** Param：0 停止；1 启动。

**响应：** 命令受理：OK，状态通过 +HFPVR 上报。

**说明：** 启动或停止远端设备的语音识别/语音助手，例如 iOS Siri。

```text
HOST> AT+HFPVR=1
xStack> OK
xStack> +HFPVR=1
```

### A2DP/AVRCP 命令

#### CMD-034 AT+A2DPSTAT - 读取 A2DP 状态

**支持状态：已实现**

**格式：** `AT+A2DPSTAT`

**参数：** 无。

**响应：** +A2DPSTAT=<0~5> 后跟 OK。

**说明：** 状态：0 不支持；1 待机；2 连接中；3 已连接；4 暂停；5 流媒体播放中。

```text
HOST> AT+A2DPSTAT
xStack> +A2DPSTAT=3
xStack> OK
```

#### CMD-035 AT+A2DPCONN - 建立 A2DP 连接

**支持状态：已实现**

**格式：** `AT+A2DPCONN[=<Param>]`

**参数：** Param：目标设备 12 位 MAC 地址；省略时回连最近一次 A2DP 设备。

**响应：** 本地连接请求受理：OK。

**说明：** 发起 A2DP Sink 连接，最终结果通过 +A2DPSTAT/+A2DPDEV 上报。

```text
HOST> AT+A2DPCONN=1C5CF226D773
xStack> OK
```

#### CMD-036 AT+A2DPDISC - 断开 A2DP 连接

**支持状态：已实现**

**格式：** `AT+A2DPDISC`

**参数：** 无。

**响应：** 本地断开请求受理：OK。

**说明：** 释放当前 A2DP 连接。

```text
HOST> AT+A2DPDISC
xStack> OK
```

#### CMD-037 AT+A2DPMUTE - A2DP 静音控制

**支持状态：部分实现**

**格式：** `AT+A2DPMUTE=<Param>`

**参数：** Param：0 取消静音；1 静音。默认 0。

**响应：** +A2DPMUTED=<Param> 后跟 OK。

**说明：** 当音频焦点不在蓝牙通道时可暂时静音。当前 xStack 的最终效果依赖 PCM 回调后的板端音频处理。

```text
HOST> AT+A2DPMUTE=1
xStack> +A2DPMUTED=1
xStack> OK
```

#### CMD-038 AT+AVRCPCFG - 读写 AVRCP 配置

**支持状态：部分实现**

**格式：** `AT+AVRCPCFG[=<Param>]`

**参数：** Param 位掩码：BIT0 曲目变化时自动获取标题/艺术家/专辑；BIT1~3 播放进度上报周期，非零值表示秒；BIT4 浏览功能；BIT5 自动获取封面。缺省值为 3。

**响应：** 查询：+AVRCPCFG=<Param> 后跟 OK；设置成功：OK。

**说明：** 当前 xStack 支持 BIT0~3；浏览和封面相关 BIT4~5 待实现。

```text
HOST> AT+AVRCPCFG=3
xStack> OK
```

#### CMD-039 AT+PLAYPAUSE - 播放/暂停切换

**支持状态：已实现**

**格式：** `AT+PLAYPAUSE`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 根据当前播放状态向远端播放器发送 Play 或 Pause。

```text
HOST> AT+PLAYPAUSE
xStack> OK
```

#### CMD-040 AT+PLAY - 播放

**支持状态：已实现**

**格式：** `AT+PLAY`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 向远端媒体播放器发送播放指令。

```text
HOST> AT+PLAY
xStack> OK
```

#### CMD-041 AT+PAUSE - 暂停

**支持状态：已实现**

**格式：** `AT+PAUSE`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 向远端媒体播放器发送暂停指令。

```text
HOST> AT+PAUSE
xStack> OK
```

#### CMD-042 AT+STOP - 停止播放

**支持状态：已实现**

**格式：** `AT+STOP`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 向远端媒体播放器发送停止指令。

```text
HOST> AT+STOP
xStack> OK
```

#### CMD-043 AT+NEXT - 下一曲

**支持状态：已实现**

**格式：** `AT+NEXT`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 向远端媒体播放器发送下一曲指令。

```text
HOST> AT+NEXT
xStack> OK
```

#### CMD-044 AT+PREV - 上一曲

**支持状态：已实现**

**格式：** `AT+PREV`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 向远端媒体播放器发送上一曲指令。

```text
HOST> AT+PREV
xStack> OK
```

#### CMD-045 AT+FORWARD - 快进

**支持状态：已实现**

**格式：** `AT+FORWARD`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 向远端媒体播放器发送快进按下/释放操作。

```text
HOST> AT+FORWARD
xStack> OK
```

#### CMD-046 AT+BACKWARD - 快退

**支持状态：已实现**

**格式：** `AT+BACKWARD`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 向远端媒体播放器发送快退按下/释放操作。

```text
HOST> AT+BACKWARD
xStack> OK
```

#### CMD-047 AT+REPEAT - 设置循环模式

**支持状态：已实现**

**格式：** `AT+REPEAT=<Param>`

**参数：** Param：0 关闭循环；1 开启循环。

**响应：** 命令受理：OK。

**说明：** 该命令使用开关形式。当前 xStack 把 0 映射为 Off，把 1 映射为 All Tracks。

```text
HOST> AT+REPEAT=1
xStack> OK
```

#### CMD-048 AT+SHUFFLE - 设置随机播放

**支持状态：已实现**

**格式：** `AT+SHUFFLE=<Param>`

**参数：** Param：0 关闭随机；1 开启随机。

**响应：** 命令受理：OK。

**说明：** 该命令使用开关形式。当前 xStack 把 0 映射为 Off，把 1 映射为 All Tracks。

```text
HOST> AT+SHUFFLE=1
xStack> OK
```

#### CMD-049 AT+GETMP - 列出远端媒体播放器

**支持状态：待实现**

**格式：** `AT+GETMP`

**参数：** 无。

**响应：** +BROWDATA 返回媒体播放器记录。

**说明：** 列出远端设备可浏览的媒体播放器。需要 AVRCP Browsing。

```text
HOST> AT+GETMP
预期> +BROWDATA=P,<索引>,<名称>,<可浏览标志>
```

#### CMD-050 AT+SETMP - 选择媒体播放器

**支持状态：待实现**

**格式：** `AT+SETMP=<Param>`

**参数：** Param：媒体播放器索引。

**响应：** 成功后通过 +BROWDATA 返回根目录信息，并返回 OK。

**说明：** 选择一个支持浏览的媒体播放器并进入其根目录。需要 AVRCP Browsing。

```text
HOST> AT+SETMP=1
预期> OK
```

#### CMD-051 AT+GETFD - 列出当前目录内容

**支持状态：待实现**

**格式：** `AT+GETFD=<Param1>,<Param2>`

**参数：** Param1：起始位置 1~65535。Param2：结束位置 1~65535，且 Param2>=Param1。

**响应：** 文件夹：+BROWDATA=F,<参数...>；媒体项：+BROWDATA=M,<参数...>。

**说明：** 列出当前浏览目录中的子文件夹和媒体条目。需要 AVRCP Browsing。

```text
HOST> AT+GETFD=1,20
预期> +BROWDATA=F,...
```

#### CMD-052 AT+SETFD - 进入或返回文件夹

**支持状态：待实现**

**格式：** `AT+SETFD=<Param>`

**参数：** Param：0 返回上一级；其他值为目标 Folder ID。

**响应：** 命令受理：OK。

**说明：** 选择并进入媒体浏览文件夹。需要 AVRCP Browsing。

```text
HOST> AT+SETFD=0
预期> OK
```

#### CMD-053 AT+GETNP - 列出正在播放列表

**支持状态：待实现**

**格式：** `AT+GETNP=<Param1>,<Param2>`

**参数：** Param1：起始位置 1~65535。Param2：结束位置 1~65535，且 Param2>=Param1。

**响应：** +BROWDATA=M,<参数...>。

**说明：** 读取 Now Playing 列表中的媒体条目。需要 AVRCP Browsing。

```text
HOST> AT+GETNP=1,20
预期> +BROWDATA=M,...
```

#### CMD-054 AT+ADDMP - 添加并播放媒体条目

**支持状态：待实现**

**格式：** `AT+ADDMP=<Param>`

**参数：** Param：Track ID。

**响应：** 命令受理：OK。

**说明：** 把选定曲目加入远端媒体播放器并开始播放。需要 AVRCP Browsing。

```text
HOST> AT+ADDMP=1234
预期> OK
```

#### CMD-055 AT+GBIPART - 主动获取专辑封面

**支持状态：待实现**

**格式：** `AT+GBIPART`

**参数：** 无。

**响应：** 命令受理：OK；状态由 +BIPSTAT/+COVERART 上报。

**说明：** 主动获取当前曲目的封面图片。需要 BIP Cover Art。

```text
HOST> AT+GBIPART
预期> OK
```

#### CMD-056 AT+SABSUP - 绝对音量增加

**支持状态：已实现**

**格式：** `AT+SABSUP`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 通过 AVRCP 增加绝对音量；最终值由 +ABSVOL 上报。

```text
HOST> AT+SABSUP
xStack> OK
```

#### CMD-057 AT+SABSDOWN - 绝对音量降低

**支持状态：已实现**

**格式：** `AT+SABSDOWN`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 通过 AVRCP 降低绝对音量；最终值由 +ABSVOL 上报。

```text
HOST> AT+SABSDOWN
xStack> OK
```

### PBAP 命令

#### CMD-058 AT+PBDOWN - 下载电话簿或通话记录

**支持状态：待实现**

**格式：** `AT+PBDOWN=<Param1>[,<Param2>[,<Param3>[,<Param4>[,<Param5>]]]]`

**参数：** Param1：0 SIM 电话簿；1 手机电话簿；2 已接；3 已拨；4 未接；5 全部通话记录；6 vCard 列表。Param2：偏移量。Param3：最大条数，电话簿默认 3000、通话记录默认 50。Param4：0 姓-中间名-名；1 名-中间名-姓；2 原始 vCard 2.1。Param5：是否需要联系人照片，0/1。

**响应：** +PBDATA 数据事件，完成时 +PBDATA=E。

**说明：** 发起 PBAP Client 下载。原文格式行只列到 Param4，但正文还定义了 Param5，本手册保留全部定义。

```text
HOST> AT+PBDOWN=1,0,3000,0,0
预期> +PBDATA=...
```

#### CMD-059 AT+PBABORT - 中止电话簿下载

**支持状态：待实现**

**格式：** `AT+PBABORT`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 中止当前 PBAP 下载操作。

```text
HOST> AT+PBABORT
预期> OK
```

#### CMD-060 AT+PBCONN - 主动建立 PBAP 连接

**支持状态：待实现**

**格式：** `AT+PBCONN`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 主动发起 PBAP Client 连接。

```text
HOST> AT+PBCONN
预期> OK
```

#### CMD-061 AT+PBDISC - 断开 PBAP 连接

**支持状态：待实现**

**格式：** `AT+PBDISC`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 释放 PBAP Client 连接。命令名称采用 AT+PBDISC；相关命名差异见“命名与格式说明”。

```text
HOST> AT+PBDISC
预期> OK
```

### SPP 命令

#### CMD-062 AT+SPPSTAT - 读取 SPP 状态

**支持状态：待实现**

**格式：** `AT+SPPSTAT`

**参数：** 无。

**响应：** +SPPSTAT=<0~3> 后跟 OK。

**说明：** 状态：0 不支持；1 待机；2 连接中；3 已连接。

```text
HOST> AT+SPPSTAT
预期> +SPPSTAT=1
预期> OK
```

#### CMD-063 AT+SPPCONN - 建立 SPP 连接

**支持状态：待实现**

**格式：** `AT+SPPCONN=<Param>`

**参数：** Param：目标设备 12 位 MAC 地址。

**响应：** 命令受理：OK。

**说明：** 发起 SPP 连接。

```text
HOST> AT+SPPCONN=1C5CF226D773
预期> OK
```

#### CMD-064 AT+SPPDISC - 断开 SPP 连接

**支持状态：待实现**

**格式：** `AT+SPPDISC`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 释放当前 SPP 连接。

```text
HOST> AT+SPPDISC
预期> OK
```

#### CMD-065 AT+SPPSEND - 通过 SPP 发送数据

**支持状态：待实现**

**格式：** `AT+SPPSEND=<Param1>,<Param2>`

**参数：** Param1：载荷长度 1~492。Param2：对应长度的 UTF-8 载荷。

**响应：** 发送受理：OK。

**说明：** 通过 SPP 发送透明数据。Android/Linux 平台原文建议使用 Socket 代替 AT 数据通道。

```text
HOST> AT+SPPSEND=10,1234567890
预期> OK
```

### GATT 命令

#### CMD-066 AT+GATTSTAT - 读取 GATT 状态

**支持状态：待实现**

**格式：** `AT+GATTSTAT`

**参数：** 无。

**响应：** +GATTSTAT=<0~3> 后跟 OK。

**说明：** 状态：0 不支持；1 待机；2 连接中；3 已连接。xStack 存在 GATT Profile 模块，但尚未接入本手册定义的 AT 命令。

```text
HOST> AT+GATTSTAT
预期> +GATTSTAT=1
预期> OK
```

#### CMD-067 AT+GATTDISC - 断开 GATT 连接

**支持状态：待实现**

**格式：** `AT+GATTDISC`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 释放当前 GATT 连接。

```text
HOST> AT+GATTDISC
预期> OK
```

#### CMD-068 AT+GATTSEND - 通过 GATT 发送数据

**支持状态：待实现**

**格式：** `AT+GATTSEND=<Param1>,<Param2>`

**参数：** Param1：载荷长度 1~492。Param2：对应长度的 UTF-8 载荷。

**响应：** 发送受理：OK。

**说明：** 通过预定义 GATT 数据通道发送数据。

```text
HOST> AT+GATTSEND=10,1234567890
预期> OK
```

### HID 命令

#### CMD-069 AT+HIDCONN - 建立 HID 连接

**支持状态：待实现**

**格式：** `AT+HIDCONN`

**参数：** 无。

**响应：** 状态通过 +HIDSTAT 上报。

**说明：** 主动建立 HID 设备连接。xStack 存在 HID/HOGP Profile 模块，但尚未接入本手册定义的 AT 命令。

```text
HOST> AT+HIDCONN
预期> OK
```

#### CMD-070 AT+HIDDISC - 断开 HID 连接

**支持状态：待实现**

**格式：** `AT+HIDDISC`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 释放当前 HID 连接。

```text
HOST> AT+HIDDISC
预期> OK
```

#### CMD-071 AT+HIDKEY - 发送 HID 遥控按键

**支持状态：待实现**

**格式：** `AT+HIDKEY=<Param>`

**参数：** Param：00 释放；01 播放/暂停；02 下一曲；03 上一曲；04 音量增加；05 音量降低。

**响应：** 命令受理：OK。

**说明：** 先发送按下键值，再发送 00 释放。间隔较短表示单击，间隔足够长表示长按。

```text
HOST> AT+HIDKEY=01
HOST> AT+HIDKEY=00
```

### iAP2 映射命令

#### CMD-072 AT+IAPSTAT - 读取 iAP2 状态

**支持状态：待实现**

**格式：** `AT+IAPSTAT`

**参数：** 无。

**响应：** +IAPSTAT=<0~3> 后跟 OK。

**说明：** 读取 iAP2 连接状态，状态字段与 SPP 状态语义一致。

```text
HOST> AT+IAPSTAT
预期> +IAPSTAT=1
预期> OK
```

#### CMD-073 AT+IAPCONN - 建立 iAP2 连接

**支持状态：待实现**

**格式：** `AT+IAPCONN=<Param>`

**参数：** Param：目标设备 12 位 MAC 地址。

**响应：** 命令受理：OK。

**说明：** 对应 AT+SPPCONN。原文只在映射说明中列出，未单独展开参数章节。

```text
HOST> AT+IAPCONN=1C5CF226D773
预期> OK
```

#### CMD-074 AT+IAPDISC - 断开 iAP2 连接

**支持状态：待实现**

**格式：** `AT+IAPDISC`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 对应 AT+SPPDISC。

```text
HOST> AT+IAPDISC
预期> OK
```

#### CMD-075 AT+IAPSEND - 通过 iAP2 发送数据

**支持状态：待实现**

**格式：** `AT+IAPSEND=<Param1>,<Param2>`

**参数：** Param1：载荷长度 1~492。Param2：对应长度的数据。

**响应：** 发送受理：OK。

**说明：** 对应 AT+SPPSEND。

```text
HOST> AT+IAPSEND=10,1234567890
预期> OK
```

### AAP 映射命令

#### CMD-076 AT+AAPSTAT - 读取 AAP 状态

**支持状态：待实现**

**格式：** `AT+AAPSTAT`

**参数：** 无。

**响应：** +AAPSTAT=<0~3> 后跟 OK。

**说明：** 读取 AAP 连接状态，状态字段与 SPP 状态语义一致。

```text
HOST> AT+AAPSTAT
预期> +AAPSTAT=1
预期> OK
```

#### CMD-077 AT+APPCONN - 建立 AAP 连接

**支持状态：待实现**

**格式：** `AT+APPCONN=<Param>`

**参数：** Param：目标设备 12 位 MAC 地址。

**响应：** 命令受理：OK。

**说明：** 主动建立 AAP 连接。命令名称保留为 AT+APPCONN；同组前缀差异见“命名与格式说明”。

```text
HOST> AT+APPCONN=1C5CF226D773
预期> OK
```

#### CMD-078 AT+AAPDISC - 断开 AAP 连接

**支持状态：待实现**

**格式：** `AT+AAPDISC`

**参数：** 无。

**响应：** 命令受理：OK。

**说明：** 对应 AT+SPPDISC。

```text
HOST> AT+AAPDISC
预期> OK
```

#### CMD-079 AT+APPSEND - 通过 AAP 发送数据

**支持状态：待实现**

**格式：** `AT+APPSEND=<Param1>,<Param2>`

**参数：** Param1：载荷长度 1~492。Param2：对应长度的数据。

**响应：** 发送受理：OK。

**说明：** 通过 AAP 通道发送数据。命令名称保留为 AT+APPSEND；同组前缀差异见“命名与格式说明”。

```text
HOST> AT+APPSEND=10,1234567890
预期> OK
```

## 5 Event 详细说明

### 通用事件

#### EVT-001 +DEVSTAT - 设备状态

**支持状态：部分实现**

**格式：** `+DEVSTAT=<Param>`

**参数：** Param 为十进制位掩码：BIT0 电源；BIT1 BR/EDR 可发现；BIT2 BLE 广播；BIT3 BR/EDR 扫描；BIT4 BLE 扫描。

**说明：** 设备电源、可发现、广播或扫描状态变化时上报。当前 xStack 主要覆盖电源、BR/EDR 可发现和 BR/EDR 扫描位，BLE 位待实现。

```text
+DEVSTAT=7
```

#### EVT-002 +PWRSTAT - 上下电过程状态

**支持状态：已实现**

**格式：** `+PWRSTAT=<Param>`

**参数：** Param：0 正在下电；1 正在上电/启动。

**说明：** 控制器进入上电或下电过程时上报。在该状态期间不建议发送依赖协议栈状态的命令。

```text
+PWRSTAT=1
```

#### EVT-003 +SCAN - 扫描结果

**支持状态：部分实现**

**格式：** `+SCAN=<Param1><FF><Param2><FF><Param3><FF><Param4><FF><Param5><FF><Param6> 或 +SCAN=E`

**参数：** Param1：索引。Param2：RSSI，-127~-1。Param3：地址类型，0 BR/EDR；1 LE Public；2 LE Random；3 支持 CarPlay 的 iOS 设备。Param4：12 位 MAC。Param5：BR/EDR 名称或 LE 广播数据。Param6：6 位 Class of Device。

**说明：** 每发现一个设备上报一条，扫描结束上报 +SCAN=E。当前 xStack 仅上报 BR/EDR 扫描结果。

```text
+SCAN=1<FF>-32<FF>0<FF>DC0D30000053<FF>BW226<FF>040680
+SCAN=E
```

#### EVT-004 +PAIRREQ - 配对确认请求

**支持状态：已实现**

**格式：** `+PAIRREQ=<Param1>,<Param2>[,<Param3>]`

**参数：** Param1：000000~999999 Passkey。Param2：远端 12 位 MAC。Param3：远端设备名称。

**说明：** SSP 数字比较需要用户确认时上报。应使用 AT+CFM 接受或拒绝。

```text
+PAIRREQ=123456,1C5CF226D773,Phone
```

#### EVT-005 +PAIRED - 配对结果

**支持状态：已实现**

**格式：** `+PAIRED=<Param1>,<Param2>`

**参数：** Param1：0 成功；1~255 失败原因。Param2：远端 12 位 MAC。

**说明：** 配对流程完成时上报结果。

```text
+PAIRED=0,1C5CF226D773
```

#### EVT-006 +MICMUTED - 麦克风静音状态

**支持状态：部分实现**

**格式：** `+MICMUTED=<Param>`

**参数：** Param：0 未静音；1 已静音。

**说明：** AT+MICMUTE 生效或状态变化时上报。当前完整音频效果仍依赖板端 SCO 路径。

```text
+MICMUTED=1
```

### HFP 事件

#### EVT-007 +HFPSTAT - HFP/呼叫状态

**支持状态：已实现**

**格式：** `+HFPSTAT=<Param1>[,<Param2>]`

**参数：** Param1：0~10，含不支持、待机、连接中、已连接、去电、来电、通话中及三方通话组合状态。Param2：状态大于 3 时可携带电话号码。

**说明：** HFP 服务级连接或呼叫状态变化时上报。

```text
+HFPSTAT=5,13265463800
```

#### EVT-008 +HFPDEV - HFP 远端设备信息

**支持状态：已实现**

**格式：** `+HFPDEV=<Param1>[<FF><Param2>]`

**参数：** Param1：远端 12 位 MAC。Param2：UTF-8 远端名称。

**说明：** HFP 连接成功并获得远端信息时上报。

```text
+HFPDEV=1C5CF226D774<FF>iPhone
```

#### EVT-009 +HFPAUDIO - HFP 语音音频状态

**支持状态：已实现**

**格式：** `+HFPAUDIO=<Param>`

**参数：** Param：0 SCO 语音断开，音频位于远端；1 SCO 语音连接，音频路由到模块。

**说明：** HFP 语音音频连接状态变化时上报。

```text
+HFPAUDIO=1
```

#### EVT-010 +HFPVR - 语音助手状态

**支持状态：已实现**

**格式：** `+HFPVR=<Param>`

**参数：** Param：0 关闭；1 开启。

**说明：** 远端语音助手状态变化时上报。原文格式行误写为 +HFPAUDIO，本手册采用 +HFPVR。

```text
+HFPVR=1
```

#### EVT-011 +HFPSIG - 远端网络信号强度

**支持状态：已实现**

**格式：** `+HFPSIG=<Param>`

**参数：** Param：0~5。

**说明：** HFP AG 上报手机网络信号强度变化时转发。

```text
+HFPSIG=5
```

#### EVT-012 +HFPNET - 远端网络运营商

**支持状态：已实现**

**格式：** `+HFPNET=<Param>`

**参数：** Param：UTF-8 运营商名称。

**说明：** HFP AG 提供运营商信息时上报。

```text
+HFPNET=China Mobile
```

#### EVT-013 +HFPROAM - 远端漫游状态

**支持状态：已实现**

**格式：** `+HFPROAM=<Param>`

**参数：** Param：0 未漫游；1 漫游。

**说明：** 远端手机漫游状态变化时上报。

```text
+HFPROAM=0
```

#### EVT-014 +HFPBATT - 远端电池电量

**支持状态：已实现**

**格式：** `+HFPBATT=<Param>`

**参数：** Param：0~5。

**说明：** 远端手机电池等级变化时上报。

```text
+HFPBATT=4
```

#### EVT-015 +HFPMANU - 远端制造商

**支持状态：已实现**

**格式：** `+HFPMANU=<Param>`

**参数：** Param：UTF-8 制造商名称。

**说明：** 获得远端设备制造商信息时上报。

```text
+HFPMANU=Apple
```

#### EVT-016 +HFPNUM - 远端本机号码

**支持状态：已实现**

**格式：** `+HFPNUM=<Param>`

**参数：** Param：ASCII 电话号码。

**说明：** 远端手机提供用户号码时上报。

```text
+HFPNUM=13800138000
```

#### EVT-017 +HFPIBR - 远端带内铃声能力

**支持状态：已实现**

**格式：** `+HFPIBR=<Param>`

**参数：** Param：0 不支持；1 支持。

**说明：** HFP 连接后报告远端是否支持带内铃声。

```text
+HFPIBR=1
```

#### EVT-018 +HFPRING - 远端来电振铃

**支持状态：已实现**

**格式：** `+HFPRING=<Param>`

**参数：** Param：0 振铃关闭；1 振铃开启。

**说明：** 来电振铃状态变化时上报。若远端不支持带内铃声，HOST 应播放本地铃声。

```text
+HFPRING=1
```

#### EVT-019 +HFPLOST - HFP 链路意外丢失

**支持状态：已实现**

**格式：** `+HFPLOST=<Param>`

**参数：** Param：HFP 设备 12 位 MAC。

**说明：** 远端关机、超出范围等导致 HFP 链路意外丢失时上报；主动断开不应作为 LOST 上报。

```text
+HFPLOST=1C5CF226D773
```

#### EVT-020 +HFPMTPX - 多路呼叫状态

**支持状态：已实现**

**格式：** `+HFPMTPX=<Param1>,<Param2>[,<Param3>]`

**参数：** Param1：呼叫索引。Param2：0 活动；1 保持；2 拨号；3 呼出振铃；4 来电；5 等待；6 挂断。Param3：可选电话号码。

**说明：** 三方通话或多呼叫状态变化时上报。

```text
+HFPMTPX=1,2,10086
```

### A2DP/AVRCP 事件

#### EVT-021 +A2DPSTAT - A2DP 状态

**支持状态：已实现**

**格式：** `+A2DPSTAT=<Param>`

**参数：** Param：0 不支持；1 待机；2 连接中；3 已连接；4 暂停；5 流媒体播放。

**说明：** A2DP 连接或流状态变化时上报。

```text
+A2DPSTAT=5
```

#### EVT-022 +A2DPDEV - A2DP 远端设备信息

**支持状态：已实现**

**格式：** `+A2DPDEV=<Param1>[<FF><Param2>]`

**参数：** Param1：远端 12 位 MAC。Param2：UTF-8 远端名称。

**说明：** A2DP 连接成功并获得远端信息时上报。

```text
+A2DPDEV=1C5CF226D773<FF>Phone
```

#### EVT-023 +AVRCPSTAT - AVRCP 状态

**支持状态：已实现**

**格式：** `+AVRCPSTAT=<Param>`

**参数：** Param：0 不支持；1 待机；2 连接中；3 已连接。

**说明：** AVRCP 控制连接状态变化时上报。

```text
+AVRCPSTAT=3
```

#### EVT-024 +A2DPLOST - A2DP 链路意外丢失

**支持状态：已实现**

**格式：** `+A2DPLOST=<Param>`

**参数：** Param：A2DP 设备 12 位 MAC。

**说明：** 远端关机、超出范围等导致 A2DP 链路意外丢失时上报。

```text
+A2DPLOST=1C5CF226D773
```

#### EVT-025 +PLAYSTAT - 播放器状态

**支持状态：已实现**

**格式：** `+PLAYSTAT=<Param>`

**参数：** Param：0 停止；1 播放；2 暂停；3 快进；4 快退。

**说明：** 远端媒体播放器状态变化时上报。

```text
+PLAYSTAT=1
```

#### EVT-026 +PLAYMODE - 循环/随机播放模式

**支持状态：已实现**

**格式：** `+PLAYMODE=<Param1>,<Param2>`

**参数：** Param1 循环：1 关闭；2 单曲；3 全部；4 分组。Param2 随机：1 关闭；2 全部；3 分组。

**说明：** 远端播放器的循环或随机模式变化时上报。

```text
+PLAYMODE=3,2
```

#### EVT-027 +TRACKSTAT - 曲目播放进度

**支持状态：已实现**

**格式：** `+TRACKSTAT=<Param1>,<Param2>,<Param3>`

**参数：** Param1：0~4 播放器状态。Param2：已播放秒数。Param3：总时长秒数。

**说明：** 按 AVRCP 配置周期上报当前曲目播放进度。

```text
+TRACKSTAT=1,54,322
```

#### EVT-028 +TRACKINFO - 曲目信息

**支持状态：已实现**

**格式：** `+TRACKINFO=<Param1><FF><Param2><FF><Param3>`

**参数：** Param1：标题。Param2：艺术家。Param3：专辑。

**说明：** 当前曲目元数据变化时上报。事件名称采用 +TRACKINFO；相关命名差异见“命名与格式说明”。

```text
+TRACKINFO=Creep<FF>Radiohead<FF>PabloHoney
```

#### EVT-029 +BROWSTAT - 媒体浏览状态

**支持状态：待实现**

**格式：** `+BROWSTAT=<Param>`

**参数：** Param：0 不支持；1 待机；2 连接中；3 已连接；4 浏览中。

**说明：** AVRCP Browsing 状态变化时上报。

```text
+BROWSTAT=4
```

#### EVT-030 +BROWDATA - 媒体浏览数据

**支持状态：待实现**

**格式：** `+BROWDATA=<类型>,<字段...>`

**参数：** 类型通常包括播放器 P、文件夹 F、媒体条目 M；具体字段随 GETMP、SETMP、GETFD、GETNP 操作变化。

**说明：** 返回媒体播放器、目录或曲目浏览结果。字段随发起的浏览命令变化，应用应结合对应操作解析。

```text
+BROWDATA=F,<FolderID>,<名称>,<可播放标志>
```

#### EVT-031 +BIPSTAT - BIP 封面传输状态

**支持状态：待实现**

**格式：** `+BIPSTAT=<Param>`

**参数：** Param：0 不支持；1 待机；2 连接中；3 已连接；4 下载中。

**说明：** 用于媒体封面图片下载过程状态。

```text
+BIPSTAT=4
```

#### EVT-032 +COVERART - 封面下载成功

**支持状态：待实现**

**格式：** `+COVERART=<Param>`

**参数：** Param：Image ID。

**说明：** 封面图片下载成功时上报。图片以 ImageID.jpg 命名并保存到指定目录。

```text
+COVERART=12345
```

#### EVT-033 +ABSVOL - 绝对音量通知

**支持状态：已实现**

**格式：** `+ABSVOL=<Param>`

**参数：** Param：0~127。

**说明：** 远端绝对音量发生变化时上报。

```text
+ABSVOL=96
```

### PBAP 事件

#### EVT-034 +PBSTAT - PBAP Client 状态

**支持状态：待实现**

**格式：** `+PBSTAT=<Param>`

**参数：** Param：0 不支持；1 待机；2 连接中；3 已连接；4 下载中。

**说明：** PBAP Client 连接或下载状态变化时上报。

```text
+PBSTAT=4
```

#### EVT-035 +PBDATA - 电话簿数据

**支持状态：待实现**

**格式：** `+PBDATA=<字段...> 或 +PBDATA=E`

**参数：** 字段随下载类型变化，包含类型、姓名、号码类型、号码、可选通话时间、联系人照片完成标志和索引。通话时间格式 YYYYMMDDThhmmss。

**说明：** 逐条返回联系人或通话记录，全部完成时返回 +PBDATA=E。字段按业务语义描述，参数编号差异见“命名与格式说明”。

```text
+PBDATA=1<FF>Jack<FF>18219146201
+PBDATA=E
```

### SPP 事件

#### EVT-036 +SPPSTAT - SPP 状态

**支持状态：待实现**

**格式：** `+SPPSTAT=<Param>`

**参数：** Param：0 不支持；1 待机；2 连接中；3 已连接。

**说明：** SPP 连接状态变化时上报。

```text
+SPPSTAT=3
```

#### EVT-037 +SPPDATA - SPP 接收数据

**支持状态：待实现**

**格式：** `+SPPDATA=<Param1>,<Param2>`

**参数：** Param1：载荷长度。Param2：载荷。

**说明：** 通过 SPP 收到远端数据时上报。Android/Linux 平台原文建议使用 Socket 代替 AT 数据通道。

```text
+SPPDATA=10,1234567890
```

### GATT 事件

#### EVT-038 +GATTSTAT - GATT 状态

**支持状态：待实现**

**格式：** `+GATTSTAT=<Param>`

**参数：** Param：0 不支持；1 待机；2 连接中；3 已连接。

**说明：** GATT 数据通道状态变化时上报。

```text
+GATTSTAT=3
```

#### EVT-039 +GATTDATA - GATT 接收数据

**支持状态：待实现**

**格式：** `+GATTDATA=<Param1>,<Param2>`

**参数：** Param1：载荷长度。Param2：载荷。

**说明：** 通过预定义 GATT 数据通道收到远端数据时上报。

```text
+GATTDATA=10,1234567890
```

### HID 事件

#### EVT-040 +HIDSTAT - HID 状态

**支持状态：待实现**

**格式：** `+HIDSTAT=<Param>`

**参数：** Param：0 不支持；1 待机；2 连接中；3 已连接。

**说明：** HID 连接状态变化时上报。

```text
+HIDSTAT=3
```

### ANCS 事件

#### EVT-041 +ANCSSTAT - ANCS 状态

**支持状态：待实现**

**格式：** `+ANCSSTAT=<Param>`

**参数：** Param：0 不支持；1 待机；2 连接中；3 已连接。

**说明：** ANCS 服务连接状态变化时上报。

```text
+ANCSSTAT=3
```

#### EVT-042 +ANCSNT - ANCS 通知摘要

**支持状态：待实现**

**格式：** `+ANCSNT=<Param>`

**参数：** Param 为十六进制字段串：字节 0~1 event_id；2~3 event_flags；4~5 category_id；6~7 category_count；8~15 notification_uid。原文写作 8~16，示例长度按 8 字节 UID 理解。

**说明：** 收到 iOS ANCS 通知时上报摘要字段。

```text
+ANCSNT=0015040C0000000B
```

#### EVT-043 +ANCSNTI - iOS 通知详细信息

**支持状态：待实现**

**格式：** `+ANCSNTI=<data>`

**参数：** data：iOS 通知详细数据。

**说明：** 收到 iOS 通知详细信息时上报。

```text
+ANCSNTI=<data>
```

#### EVT-044 +ANCSNTA - Android 通知详细信息

**支持状态：待实现**

**格式：** `+ANCSNTA=<data>`

**参数：** data：Android 通知详细数据。

**说明：** 收到 Android 通知详细信息时上报。事件名称采用 +ANCSNTA；相关命名差异见“命名与格式说明”。

```text
+ANCSNTA=<data>
```

### iAP2 映射事件

#### EVT-045 +IAPSTAT - iAP2 状态

**支持状态：待实现**

**格式：** `+IAPSTAT=<Param>`

**参数：** Param：0 不支持；1 待机；2 连接中；3 已连接。

**说明：** iAP2 连接状态变化时上报，状态字段与 SPP 状态语义一致。

```text
+IAPSTAT=3
```

#### EVT-046 +IAPDATA - iAP2 接收数据

**支持状态：待实现**

**格式：** `+IAPDATA=<Param1>,<Param2>`

**参数：** Param1：载荷长度。Param2：载荷。

**说明：** 对应 +SPPDATA。

```text
+IAPDATA=10,1234567890
```

### AAP 映射事件

#### EVT-047 +AAPSTAT - AAP 状态

**支持状态：待实现**

**格式：** `+AAPSTAT=<Param>`

**参数：** Param：0 不支持；1 待机；2 连接中；3 已连接。

**说明：** AAP 连接状态变化时上报，状态字段与 SPP 状态语义一致。

```text
+AAPSTAT=3
```

#### EVT-048 +AAPDATA - AAP 接收数据

**支持状态：待实现**

**格式：** `+AAPDATA=<Param1>,<Param2>`

**参数：** Param1：载荷长度。Param2：载荷。

**说明：** 对应 +SPPDATA。

```text
+AAPDATA=10,1234567890
```

## 6 命名与格式说明

- 命令名称统一采用 AT+HFPMCAL 和 AT+AVRCPCFG；早期示例中命令名后的空格不属于协议内容。
- HFP 状态值范围统一为 0~10。
- 语音助手事件统一采用 +HFPVR；早期格式说明中的 +HFPAUDIO 为命名错误。
- 曲目信息事件统一采用 +TRACKINFO；+MEDIAINFO 仅为早期章节标题中的名称。
- PBAP 断开命令统一采用 AT+PBDISC；AT+PBDSIC 为早期章节标题中的拼写错误。
- Android 通知详细事件统一采用 +ANCSNTA；+ANCSNTI 为早期格式说明中的拼写错误。
- AAP 命令沿用 AT+APPCONN 和 AT+APPSEND。其 APP 前缀与同组 AAP 前缀不一致，当前按既有命令字符串保留并标记为待实现。
- AT+PBDOWN 的格式包含 Param1~Param5；Param5 用于指定是否需要联系人照片。
- +PBDATA 按字段语义进行说明，不根据重复的参数编号推断未确认的数据布局；<FF> 统一表示 ASCII Form Feed 字节 0x0C。
