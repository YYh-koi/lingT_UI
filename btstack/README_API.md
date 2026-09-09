# xStack C API 说明

`btstack_api/` 是 `btstack/` 的独立副本。协议栈、Profile、GAP、传输层和
GPCV2178 适配层继续使用原实现；应用入口改为 `bt_xstack_*` 类型化 C API，
API 工程不编译 `btstack_api/at/` 和旧的 `bt_xstack_manager.c`。

公共入口为 `bt_xstack_api/include/api/bt_xstack.h`，分类头文件为：

- `bt_xstack_types.h`
- `bt_xstack_service.h`
- `bt_xstack_phone.h`
- `bt_xstack_audio.h`
- `bt_xstack_phonebook.h`

控制和设置接口是异步接口。返回 `BT_XSTACK_OK` 仅表示参数有效且请求已进入
内部静态队列；执行结果不发送 `COMMAND_COMPLETE`，执行失败也不额外通知。
应用通过后续 service、connection、phone、audio 状态事件判断实际结果。
所有 `bt_xstack_get_*` 接口均为同步 Getter，从线程安全状态快照读取。

## AT 命令到 C API 的逐项映射

| AT 命令/形式 | C API |
|---|---|
| `AT` | `bt_xstack_is_ready()` |
| `PROFILE?` / `PROFILE=<mask>` | `bt_xstack_get_profile_mask()` / `bt_xstack_set_profile_mask()` |
| `VER?` | `bt_xstack_get_version()` |
| `BTEN=0/1` | `bt_xstack_disable()` / `bt_xstack_enable()` |
| `ADDR?` | `bt_xstack_get_local_address()` |
| `NAME?` / `NAME=<name>` | `bt_xstack_get_local_name()` / `bt_xstack_set_local_name()` |
| `PIN?` / `PIN=<pin>` | `bt_xstack_get_pin()` / `bt_xstack_set_pin()` |
| `SSP?` / `SSP=<mode>` | `bt_xstack_get_ssp_mode()` / `bt_xstack_set_ssp_mode()` |
| `CFM=<addr>,<accept>` | `bt_xstack_pairing_confirm()` |
| `PAIR` / `PAIR=<enable>` | `bt_xstack_set_discoverable()` |
| `PAGE` / `PAGE=<enable>` | `bt_xstack_set_connectable()` |
| `COD?` / `COD=<cod>` | `bt_xstack_get_class_of_device()` / `bt_xstack_set_class_of_device()` |
| `PLIST?` | `bt_xstack_get_paired_device_count()` + `bt_xstack_get_paired_device()` |
| `PLIST=<index/address/all>` | `bt_xstack_clear_paired_device_by_index()` / `bt_xstack_clear_paired_device_by_address()` / `bt_xstack_clear_paired_devices()` |
| `TDL?` / `TDL=<mode>` | `bt_xstack_get_tdl_mode()` / `bt_xstack_set_tdl_mode()` |
| `SCAN=<seconds>[,<filter>]` | `bt_xstack_scan_start()` |
| 停止扫描 | `bt_xstack_scan_stop()` |
| `DSCA` | `bt_xstack_disconnect_all()` |
| `MICMUTE=<enable>` | `bt_xstack_set_microphone_mute()`；查询用 `bt_xstack_get_microphone_mute()` |
| `SPKVOL?` / `SPKVOL=<volume>` | `bt_xstack_get_speaker_volume()` / `bt_xstack_set_speaker_volume()` |
| `HFPCFG?` / `HFPCFG=<config>` | `bt_xstack_get_hfp_config()` / `bt_xstack_set_hfp_config()` |
| `HFPSTAT?` | `bt_xstack_get_hfp_state()` |
| `HFPCONN` / `HFPCONN=<addr>` | `bt_xstack_connect_last(HFP)` / `bt_xstack_connect(addr, HFP)` |
| `HFPDISC` | `bt_xstack_disconnect(HFP)` |
| `HFPDIAL` / `HFPDIAL=<number>` | `bt_xstack_hfp_redial()` / `bt_xstack_hfp_dial()` |
| `HFPDTMF=<code>` | `bt_xstack_hfp_send_dtmf()` |
| `HFPANSW` | `bt_xstack_hfp_answer()` |
| `HFPCHUP` | `bt_xstack_hfp_hang_up()` |
| `HFPMCAL=<action>` | `bt_xstack_hfp_multiparty_control()` |
| `HFPADTS` / `HFPADTS=<connected>` | `bt_xstack_hfp_toggle_audio()` / `bt_xstack_hfp_set_audio_connected()` |
| `HFPVR=<enable>` | `bt_xstack_hfp_set_voice_recognition()` |
| `A2DPSTAT?` | `bt_xstack_get_a2dp_state()` |
| `A2DPCONN` / `A2DPCONN=<addr>` | `bt_xstack_connect_last(A2DP)` / `bt_xstack_connect(addr, A2DP)` |
| `A2DPDISC` | `bt_xstack_disconnect(A2DP)` |
| `A2DPMUTE=<enable>` | `bt_xstack_set_a2dp_mute()`；查询用 `bt_xstack_get_a2dp_mute()` |
| `PLAYPAUSE` | `bt_xstack_avrcp_play_pause()` |
| `PLAY` | `bt_xstack_avrcp_play()` |
| `PAUSE` | `bt_xstack_avrcp_pause()` |
| `STOP` | `bt_xstack_avrcp_stop()` |
| `NEXT` | `bt_xstack_avrcp_next()` |
| `PREV` | `bt_xstack_avrcp_previous()` |
| `FORWARD` | `bt_xstack_avrcp_fast_forward()` |
| `BACKWARD` | `bt_xstack_avrcp_rewind()` |
| `REPEAT=<enable>` | `bt_xstack_avrcp_set_repeat()` |
| `SHUFFLE=<enable>` | `bt_xstack_avrcp_set_shuffle()` |
| `SABSUP` | `bt_xstack_avrcp_volume_up()` |
| `SABSDOWN` | `bt_xstack_avrcp_volume_down()` |

连接接口的 Profile 参数使用 `BT_XSTACK_PROFILE_HFP_HF`、
`BT_XSTACK_PROFILE_A2DP_SINK` 或 `BT_XSTACK_PROFILE_AVRCP`。

## 初始化示例

```c
#include "bt_xstack.h"

static void on_service(const bt_xstack_service_event_t *event, void *ctx)
{
    (void)ctx;
    /* 根据 event->id 更新应用状态。 */
}

void app_bt_start(void)
{
    bt_xstack_config_t config = {0};

    config.service_callback = on_service;
    (void)bt_xstack_init(&config);
    (void)bt_xstack_enable();
}
```

## PBAP 通讯录与通话记录

`bt_xstack_phonebook_sync()`、`bt_xstack_phonebook_cancel()` 和
`bt_xstack_phonebook_authenticate()` 都是异步请求；
`bt_xstack_phonebook_get_state()` 只读取线程安全的状态快照。HFP 上报
`BT_XSTACK_PHONE_EVENT_DEVICE` 后不会自动启动同步，应用自行调用
`bt_xstack_phonebook_sync()` 并传入事件中的设备地址。HFP `LOST` 由内部触发清理，
应用不需要为连接丢失额外调用 cancel。

`categories` 是位掩码，可传入单项、用 `|` 组合任意多项，也可直接使用
`BT_XSTACK_PHONEBOOK_ALL`。可选位是 `BT_XSTACK_PHONEBOOK_CONTACTS`、
`BT_XSTACK_PHONEBOOK_INCOMING`、`BT_XSTACK_PHONEBOOK_OUTGOING` 和
`BT_XSTACK_PHONEBOOK_MISSED`。每次 mask 同步只建立一条 PBAP 连接，并固定按
`CONTACTS`、`INCOMING`、`OUTGOING`、`MISSED` 的顺序处理被选中的分类；同一时间
只运行一个 mask 同步。

`bt_xstack_phonebook_sync()` 的返回值是 queue admission status，应用必须检查它是否为
`BT_XSTACK_OK`；成功只表示请求已经进入内部队列，并不表示 PBAP 同步已经完成。下面的
应用示例在 HFP DEVICE 事件中组合拉取联系人和未接来电；当前 demo 的默认策略则传入
`BT_XSTACK_PHONEBOOK_ALL` 拉取全部四类。已有同步处于 active 状态时，再次调用仍可能
成功入队并返回 `BT_XSTACK_OK`。API 任务（API task）处理请求并交给 BTstack run loop；
若同步仍忙，则生成 `BT_XSTACK_PHONEBOOK_EVENT_ERROR`，其中 `status` 为
`BT_XSTACK_PHONEBOOK_STATUS_BUSY`；事件任务（event task）通过 `phonebook_callback`
逐条上报这些结果。

PBAP 不创建专用队列或任务。解析后的联系人和通话记录会深拷贝到现有通用事件队列，
再由现有事件任务通过 `phonebook_callback` 一条条上报。
队列为控制事件保留固定空间；记录到达背压阈值时 PBAP 暂停读取，消费后再恢复。
因此应用应在回调中复制记录到自己的存储并尽快返回，不要长期持有事件指针，也不要
在回调中执行慢速 Flash 写入。同步由应用显式调用 `bt_xstack_phonebook_sync()` 发起，
内部只负责排队、传输与连接清理。

公共记录字段是有界的：`BT_XSTACK_PHONEBOOK_NAME_MAX` 为 48 字节，
`BT_XSTACK_PHONEBOOK_NUMBER_MAX` 为 32 字节，每个联系人最多保存
`BT_XSTACK_PHONEBOOK_NUMBER_COUNT`（4）个号码，
`BT_XSTACK_PHONEBOOK_TIMESTAMP_MAX` 为 24 字节。这些常量都表示不含结尾 NUL
的最大有效内容长度；对应字符数组额外保留一个字节给 `\0`。联系人或通话记录的
`truncated != 0` 表示解析时至少一个字段因上述容量限制被截断。回调记录内存只在
回调期间有效，应用必须先复制再持久化，并可根据 `truncated` 标记数据不完整。

```c
#include "bt_xstack.h"

static void on_phone(const bt_xstack_phone_event_t *event, void *ctx)
{
    bt_xstack_result_t admission_status;

    (void)ctx;
    if (event->id == BT_XSTACK_PHONE_EVENT_DEVICE) {
        admission_status = bt_xstack_phonebook_sync(
            &event->address,
            BT_XSTACK_PHONEBOOK_CONTACTS |
            BT_XSTACK_PHONEBOOK_MISSED);
        if (admission_status != BT_XSTACK_OK) {
            /* Handle queue admission failure: report it or retry later. */
        }
    }
}

static void on_phonebook(const bt_xstack_phonebook_event_t *event, void *ctx)
{
    (void)ctx;
    if (event->id == BT_XSTACK_PHONEBOOK_EVENT_CONTACT) {
        /* Copy event->data.contact into application-owned storage here. */
    } else if (event->id == BT_XSTACK_PHONEBOOK_EVENT_AUTH_REQUIRED) {
        (void)bt_xstack_phonebook_authenticate("1234");
    }
}

void app_bt_start(void)
{
    bt_xstack_config_t config = {0};
    config.phone_callback = on_phone;
    config.phonebook_callback = on_phonebook;
    (void)bt_xstack_init(&config);
    (void)bt_xstack_enable();
}
```

成功时先收到 `STARTED`，随后是联系人/通话记录；每个选中分类处理完毕后，
`CATEGORY_COMPLETED` 表示该分类完成。最终 `COMPLETED` 仅在 PBAP 连接关闭且状态已回到
`IDLE` 后上报。`event.category` 的类型是 `bt_xstack_phonebook_category_t`，表示当前事件
实际对应的单个分类，取值为 `BT_XSTACK_PHONEBOOK_CATEGORY_CONTACTS`、
`BT_XSTACK_PHONEBOOK_CATEGORY_INCOMING`、`BT_XSTACK_PHONEBOOK_CATEGORY_OUTGOING` 或
`BT_XSTACK_PHONEBOOK_CATEGORY_MISSED`，而不是位掩码；始终满足
`event.category != BT_XSTACK_PHONEBOOK_ALL`。若手机要求 OBEX 认证，收到 `AUTH_REQUIRED` 后调用
`bt_xstack_phonebook_authenticate()` 提交最长 32 字节的密码；应用可调用
`bt_xstack_phonebook_cancel()` 终止同步，并可随时通过 `bt_xstack_phonebook_get_state()`
读取状态快照。`ERROR` 的 `status` 区分拒绝、连接失败、协议错误、取消、连接丢失和认证超时。

分类返回 `BT_XSTACK_PHONEBOOK_STATUS_UNSUPPORTED`（远端 OBEX Not Found）时，仍先上报该分类的
`CATEGORY_COMPLETED`，再继续处理后续选中分类，最终同步结果为 partial。分类返回
`BT_XSTACK_PHONEBOOK_STATUS_DENIED`，或发生 `BT_XSTACK_PHONEBOOK_STATUS_PROTOCOL_ERROR` 时，
则先上报当前分类的 `CATEGORY_COMPLETED`，随后关闭 PBAP 连接并上报唯一的 `ERROR`；本次同步
不会继续后续分类，状态在最终事件前回到 `IDLE`。
In status terms, `UNSUPPORTED` is non-fatal and continues later selected categories;
`DENIED` and `PROTOCOL_ERROR` are fatal and do not continue the current sync.

内置 PBAP/OBEX 源码以 BTstack v1.6.2 为基线。`src/classic/pbap_client.c` 额外应用一个经过
审计的本地 status-preservation patch，用于保留 Not Found 和认证拒绝的 OBEX 状态；补丁、
基线 SHA-256 和 patched SHA-256 记录在 `btstack_api/patches/README.md`。其余受管上游文件仍按
v1.6.2 原始 SHA-256 验证。

## 构建与产物

执行：

```powershell
Project\WirelessLibraries\BTstack_API_Library\build_btstack_api.ps1
```

输出目录为 `gplib/liba_wifi_src/atbm/atbm6165/bt_api/`，包含
`libxstack_api.a`、原有六个公共头文件，以及 BLE/GATT 公共头
`bt_xstack_le.h`、`bt_xstack_gatt.h`。
