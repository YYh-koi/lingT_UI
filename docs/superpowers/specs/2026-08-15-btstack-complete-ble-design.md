# BTstack 完整基础 BLE 设计

## 1. 目标

在现有 BTstack 公共层、Profile Manager、API/AT 双前端和平台 Worker 架构上，补齐一套可实际使用的基础 BLE 能力：

- Legacy BLE 广播与 Scan Response；
- LE 扫描与广播报告；
- Central 和 Peripheral 单连接；
- Just Works、RAM Bond、Passkey/数字确认事件和隐私地址；
- GATT Client 服务/特征发现、读写和订阅；
- GATT Server 有界动态注册、读写、Notify 和 Indicate；
- HOGP Device 接入 GATT Server 与 Profile Manager；
- 公共 `bt_xstack_*` API 和 AT 前端共享同一实现；
- API、AT、Legacy 三种库继续二选一构建。

首版只实现 Legacy BLE、1M PHY、31 字节广播和单条 LE 连接，不实现扩展广播、多 PHY、并发多连接或裸机平台。

## 2. 当前问题

当前 BLE 层主要是接口骨架：

- `btstack_gap_adv.c` 和 `btstack_gap_privacy.c` 初始化为空实现；
- `btstack_gap_scan.c` 实际只实现 Classic Inquiry；
- `btstack_gatt.c` 只保存少量元数据，数据库生成和传输返回 `NOT_SUPPORTED`；
- `btstack_hogp_device.c` 未绑定真实 ATT/GATT Server；
- Profile Manager 拒绝所有非零 `le_profile_mask`；
- 公共 API/AT 没有完整 BLE GAP 和 GATT 控制面；
- ATT Server、GATT Client、SM、LE Device DB 等上游 BLE 源未完整接入工程。

## 3. 选择的架构

采用分层补齐，不把原生 BTstack BLE API直接暴露给应用。

```text
应用
 ├─ bt_xstack BLE API
 └─ BLE AT 命令
          │
          ▼
  Core Dispatcher
          │
          ▼
  Profile Manager
   ├─ LE GAP 状态
   ├─ GATT Client
   ├─ GATT Server
   └─ HOGP Device
          │
          ▼
   BTstack ATT/GATT/SM/HCI
          │
          ▼
内部事件 → Profile Manager → Event Mapper → API callback / AT 上报
```

边界规则：

- Dispatcher 只校验并分派请求，不持有 BLE 业务状态；
- Adapter 是通用 GAP 公共入口；
- GAP 负责广播、扫描、LE 连接、安全和隐私；
- 每个 GATT/Profile 文件订阅并解析自身原生事件；
- Profile Manager 负责 Profile 生命周期、单连接状态和跨模块联动；
- Event Mapper 只做稳定公共事件转换；
- API/AT 前端不直接调用 `gap_*`、`sm_*`、`gatt_client_*` 或 `att_server_*`。

## 4. GAP 文件收敛

GAP 最终收敛为：

```text
gap/
├─ btstack_gap.c          公共入口、统一状态、唯一 HCI 事件注册
├─ btstack_gap_classic.c  Classic 扫描、连接、配对和可发现配置
├─ btstack_gap_le.c       广播、LE 扫描、连接、SM、Bond 和隐私
└─ btstack_gap_ad_data.c  无状态广播数据构建工具
```

现有以下文件合并后删除：

- `btstack_gap_adv.c`；
- `btstack_gap_privacy.c`；
- `btstack_gap_link.c`；
- `btstack_gap_scan.c`；
- `btstack_gap_security.c`。

`btstack_adapter_setup()` 只调用一次 `btstack_gap_init()`。`btstack_gap_init()` 根据 `BTSTACK_CLASSIC` 和 `BTSTACK_LE` 初始化相应功能，不再暴露零散的 adv/privacy 初始化函数。

## 5. BLE GAP 能力

### 5.1 广播

支持配置：

- advertising interval min/max；
- connectable/scannable advertising type；
- own address type；
- channel map 和 filter policy；
- advertising data，最多 31 字节；
- scan response data，最多 31 字节；
- start/stop。

配置由 GAP 深拷贝保存。Stack 未工作时只允许配置，不允许启动。连接建立后停止广播；断开后是否恢复由调用方显式决定，不做隐藏自动重启。

### 5.2 扫描

Classic Inquiry 保持现有行为，新 LE 接口使用独立的 `bt_xstack_le_scan_*` 命名。LE Scan 支持：

- active/passive；
- interval/window；
- duplicate filter；
- start/stop；
- address、address type、RSSI、advertising event type 和原始 AD 数据上报。

Advertising、Scanning、Connecting 三种过程互斥。

### 5.3 LE 连接

首版只保存一条连接：

- Central 或 Peripheral role；
- peer address 和 address type；
- connection handle；
- connection interval、latency 和 supervision timeout；
- negotiated MTU；
- pairing、bond 和 encryption 状态。

支持 Central connect/cancel、双方 disconnect 和连接参数更新事件。第二条连接请求返回 `BUSY`。

### 5.4 安全、RAM Bond 和隐私

默认策略为 Just Works + Bonding：

- 初始化 Security Manager；
- 默认允许 Just Works；
- 数字确认、Passkey 显示/输入通过事件交给应用；
- Bond 使用固定容量 RAM Device DB，只在当前开机周期有效；
- 默认使用可解析私有地址并定期更新；
- 删除 Bond 时同步清除 RAM 记录和 resolving list；
- Bond 容量默认 4，满时返回明确状态，不静默覆盖。

首版不写入 GPCV2178 Flash。掉电或复位后 Bond 清空，Peer 需要重新配对；持久 Bond 等平台提供经过分区确认的 storage backend 后再接入。

## 6. GATT Client

`btstack_gatt_client.c` 独立实现，并由 `BTSTACK_GATT_CLIENT` 控制。

首版支持：

- 发现全部 Primary Service；
- 按 UUID16/UUID128 发现 Service；
- 发现 Service 内 Characteristic；
- 发现 Descriptor；
- 按 value handle 读值；
- Write Request 和 Write Without Response；
- 写 CCCD 并订阅 Notification/Indication；
- 取消订阅；
- MTU 和操作完成事件。

同一时间只允许一个异步 GATT Client 操作。操作开始时复制必要参数和写数据。忙时返回 `BUSY`；断线时当前操作只上报一次 `DISCONNECTED/CANCELED` 完成事件。

## 7. GATT Server

`btstack_gatt_server.c` 独立实现，并由 `BTSTACK_GATT_SERVER` 控制。

采用有界动态注册：

1. 开蓝牙前添加 Service；
2. 添加 Characteristic；
3. 添加 Descriptor；
4. 调用 `build()` 生成 ATT DB；
5. build 后数据库结构冻结；
6. 运行期只允许读写 value、Notify 和 Indicate。

ATT 数据库由 SDK 写入固定缓冲区，不使用上游 `att_db_util` 的动态扩容。Characteristic value 使用共享固定 value pool。Server 读操作直接读取缓存 value；Peer 写入先经过权限和长度检查，成功复制后再上报应用事件。

支持：

- UUID16 和 UUID128；
- READ、WRITE、WRITE_WITHOUT_RESPONSE、NOTIFY、INDICATE；
- 无安全、加密、认证三档权限；
- CCCD 状态；
- set/get value；
- Notify/Indicate；
- Indication complete；
- Peer write 和 subscription change 事件。

## 8. HOGP Device

`btstack_hogp_device.c` 基于 GATT Server 注册 HID、Battery 和 Device Information 服务，不维护第二套 ATT Server。

状态条件：

- LE connected；
- 所需加密完成；
- Report Characteristic 已建立；
- 相应 CCCD 已订阅；
- 满足以上条件后进入 `READY`。

只有 READY 时允许发送 Input Report。断线后清除 connection handle、subscription 和 ready 状态，但保留 Report Map、PNP ID 和注册好的数据库结构。

## 9. Profile Manager

新增运行期 LE mask：

```c
BTSTACK_PROFILE_MANAGER_LE_GATT_CLIENT
BTSTACK_PROFILE_MANAGER_LE_GATT_SERVER
BTSTACK_PROFILE_MANAGER_LE_HOGP_DEVICE
BTSTACK_PROFILE_MANAGER_LE_ALL
```

Profile Manager 保存单份 LE 状态，并负责：

- 按 mask 初始化 GATT Client/Server/HOGP；
- 将 GAP connection/security 状态传递给 GATT 和 HOGP；
- 断线时终止 GATT Client 操作并重置 Server subscription；
- 将 Profile 事件交给 Event Mapper；
- 防止重复初始化和不兼容配置。

`BTSTACK_HOGP_DEVICE` 依赖 `BTSTACK_GATT_SERVER`。Profile Manager 不实现 GATT 协议细节。

## 10. 公共 API

BLE 公共接口使用明确的 `bt_xstack_le_*` 命名，避免与 Classic Scan 混淆。

### 10.1 GAP API

```c
bt_xstack_le_adv_configure(...)
bt_xstack_le_adv_start(void)
bt_xstack_le_adv_stop(void)
bt_xstack_le_scan_start(...)
bt_xstack_le_scan_stop(void)
bt_xstack_le_connect(...)
bt_xstack_le_disconnect(void)
bt_xstack_le_pair(void)
bt_xstack_le_pairing_confirm(...)
bt_xstack_le_passkey_reply(...)
```

### 10.2 GATT Server API

```c
bt_xstack_gatt_server_add_service(...)
bt_xstack_gatt_server_add_characteristic(...)
bt_xstack_gatt_server_add_descriptor(...)
bt_xstack_gatt_server_build(void)
bt_xstack_gatt_server_set_value(...)
bt_xstack_gatt_server_get_value(...)
bt_xstack_gatt_server_notify(...)
bt_xstack_gatt_server_indicate(...)
```

### 10.3 GATT Client API

```c
bt_xstack_gatt_client_discover_services(...)
bt_xstack_gatt_client_discover_characteristics(...)
bt_xstack_gatt_client_discover_descriptors(...)
bt_xstack_gatt_client_read(...)
bt_xstack_gatt_client_write(...)
bt_xstack_gatt_client_subscribe(...)
bt_xstack_gatt_client_unsubscribe(...)
```

运行期函数返回值只表示请求是否被固定 request ring 接纳；真正结果通过事件上报。

## 11. 事件模型

在公共 envelope 中新增：

```c
BT_XSTACK_EVENT_DOMAIN_LE
BT_XSTACK_EVENT_DOMAIN_GATT
```

LE 和 GATT 事件均使用 `type + union`，不将所有可选字段平铺到同一结构体。主要事件包括：

- LE advertising started/stopped；
- scan result/complete；
- connecting/connected/disconnected；
- pairing request/complete；
- encryption/MTU/connection parameter update；
- GATT service/characteristic/descriptor result；
- value/read/notification/indication；
- write/subscribe/operation complete；
- Server peer write/subscription/indication complete。

所有事件在进入固定 ring 前深拷贝。原生 BTstack Run Loop 不直接调用应用 callback。

## 12. AT 前端

AT 版与 API 版共享相同请求 ID、Dispatcher、Profile Manager、GAP/GATT 实现和事件对象。

AT 命令按功能分组：

```text
AT+LEADV...
AT+LESCAN...
AT+LECONN...
AT+LEPAIR...
AT+GATTS...
AT+GATTC...
```

异步事件统一使用：

```text
+LEADV:
+LESCAN:
+LECONN:
+LEPAIR:
+GATTS:
+GATTC:
```

二进制 UUID 和 value 使用十六进制文本。命令返回只表示同步解析/入队结果；操作最终状态仍由异步事件报告。AT Worker 不轮询，不新增 BLE task。

## 13. 固定容量和内存

默认值：

```c
BTSTACK_LE_CONNECTION_MAX          1
BTSTACK_GATT_SERVICE_MAX           4
BTSTACK_GATT_CHARACTERISTIC_MAX    8
BTSTACK_GATT_DESCRIPTOR_MAX        8
BTSTACK_GATT_DB_MAX             1024
BTSTACK_GATT_VALUE_POOL_MAX      1024
BTSTACK_GATT_VALUE_MAX           244
BTSTACK_LE_BOND_MAX                4
```

所有值均可在唯一的 `btstack_config.h` 中覆盖，并有编译期范围检查。SDK 新增的 BLE 包装层不直接分配 heap，不新增 task 或动态 queue；继续使用现有 Stack Worker、API/Event Worker 或 AT Worker 与 `btstack_ring`。上游 BTstack 内部仍遵循当前目标配置的 allocator，但单连接、单 GATT Client 操作和固定数据库边界保证本次功能不会引入无界并发分配。

## 14. 功能开关和构建

```text
BTSTACK_LE
 ├─ 基础 LE GAP、连接、SM、Bond、隐私
 ├─ BTSTACK_GATT_CLIENT
 └─ BTSTACK_GATT_SERVER
      └─ BTSTACK_HOGP_DEVICE
```

规则：

- `BTSTACK_LE=0` 时 LE Profile 必须关闭；
- `BTSTACK_HOGP_DEVICE=1` 时 `BTSTACK_GATT_SERVER` 必须为 1；
- GATT Client 和 Server 可独立选择；
- API、AT、Legacy 三个库各自只选择一个前端，但共享同一 BLE common 源；
- 工程加入所需上游 ATT、GATT Client、SM、LE Device DB 和依赖源；
- 不引入第二份 `btstack_config.h`。

## 15. 错误处理

统一使用现有状态语义，并补足 BLE 所需映射：

```text
INVALID_ARGUMENT
NOT_READY
BUSY
NOT_FOUND
NO_MEMORY
NOT_SUPPORTED
TIMEOUT
SECURITY_ERROR
DISCONNECTED
```

原则：

- 固定容量满时返回 `NO_MEMORY`，不覆盖旧数据；
- 状态不允许时返回 `NOT_READY` 或 `BUSY`；
- 原生异步操作只产生一个最终完成事件；
- 断线、取消和超时必须释放 operation 状态；
- API/AT 队列满与协议操作失败分开报告；
- AT 同步错误映射为 `ERROR:<status>`，异步失败由 BLE/GATT 事件报告。

## 16. 验证策略

测试采用真实生产源和最小上游边界 stub，覆盖：

1. GAP 广播配置、31 字节边界、启停与互斥；
2. LE 扫描报告、duplicate filter、停止和 complete；
3. Central/Peripheral 单连接、第二连接 BUSY、断线清理；
4. Just Works、数字确认、RAM Bond 保存/删除和隐私地址；
5. GATT Server 容量、数据库冻结、读写、CCCD、Notify/Indicate；
6. GATT Client 发现、读写、订阅、忙、超时和断线取消；
7. HOGP 注册、加密、订阅、READY 和 Report；
8. Profile Manager LE mask、依赖、初始化顺序和联动；
9. API/AT 请求映射、深拷贝、事件 union 和满 ring；
10. 配置矩阵：LE off、Client only、Server only、Server+HOGP、all on；
11. API/AT/Legacy 三库 ARM 强制重建与符号/成员审计；
12. 主固件链接和 RAM/ROM 增量记录。

板端验收至少覆盖 Android/iPhone 扫描、连接、Just Works RAM Bond、同一次开机内断线重连、复位后重新配对、GATT 发现/读写/通知，以及 HOGP Input Report。Host 测试和交叉编译不能替代真实控制器与手机互操作测试。

## 17. 非目标

首版不包含：

- BLE 5 Extended Advertising；
- 2M/Coded PHY；
- 多连接；
- Mesh、ANCS 或其他新增 LE Profile；
- 运行期修改 GATT 数据库结构；
- API 与 AT 前端在同一固件共存；
- 裸机平台实现。
- Bond 掉电持久化和未经确认的 Flash 分区写入。
