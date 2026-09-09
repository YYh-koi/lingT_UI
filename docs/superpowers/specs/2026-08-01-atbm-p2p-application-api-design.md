# ATBM P2P 与应用接口完善设计

## 目标

参考 ATM630 正芯工程的
`amt630hv100-freertos/lib/wlan/atbm_wifi6/iot_wifi_atbm6165.c`，完善 GPCV2178
上的 ATBM6165 P2P GO 模式和应用层接口。

本次采用“能力对齐”，继续扩展现有 `atbm_wifi_*` 公共接口，不直接引入 Amazon
FreeRTOS 的 `iot_wifi.h`、`WIFI_*` ABI 或 FreeRTOS-TCP。GP 平台继续使用现有
Generalplus SDIO 适配层、LwIP 和 DHCP Server。

## 参考实现边界

ATM630 文件中可复用的是以下已落地流程：

- STA、AP、P2P GO 的启动入口；
- STA 与 P2P GO 在不重新下载固件的情况下切换；
- P2P 配置、P2P 任务启动、GO 启动的调用顺序；
- P2P 停止后重新启动 STA 接口的顺序。

参考文件中的 `WIFI_GetRSSI()`、`WIFI_GetConnectionInfo()`、
`WIFI_GetStationList()`、`WIFI_StartDisconnectStation()` 等函数仍是空桩，不能直接
复制。GP 版本使用 ATBM 驱动现有底层能力实现等价功能；没有可靠底层支持的国家码、
功耗模式、统计信息、能力查询、网络配置保存等接口不在本次范围内。

## 公共接口兼容性

保留现有 `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi.h` 中的枚举值、结构体和
函数签名，不改变已有应用的源代码和 ABI。

新增公共类型：

```c
typedef struct {
    char ssid[ATBM_WIFI_SSID_MAX_LEN + 1U];
    unsigned char bssid[ATBM_WIFI_MAC_ADDR_LEN];
    int channel;
    int rssi;
    int phy_rate;
} atbm_wifi_connection_info_t;

typedef struct {
    unsigned char ip[4];
    unsigned char netmask[4];
    unsigned char gateway[4];
} atbm_wifi_ip_info_t;

typedef struct {
    unsigned char mac[ATBM_WIFI_MAC_ADDR_LEN];
    int rssi;
} atbm_wifi_client_info_t;
```

IP 地址使用 4 字节网络顺序数组，避免公共头文件暴露 LwIP 类型。

新增公共函数：

```c
int atbm_wifi_is_connected(int *connected);
int atbm_wifi_get_connection_info(atbm_wifi_connection_info_t *info);
int atbm_wifi_get_rssi(int *rssi);
int atbm_wifi_get_ip_info(atbm_wifi_ip_info_t *info);

int atbm_wifi_get_clients(atbm_wifi_client_info_t *clients,
                          unsigned int max_clients,
                          unsigned int *client_count);
int atbm_wifi_disconnect_client(
    const unsigned char mac[ATBM_WIFI_MAC_ADDR_LEN]);
```

接口行为：

- `atbm_wifi_is_connected()` 仅在 STA 模式查询关联状态；未关联返回成功且
  `connected=0`。
- 连接信息和 RSSI 仅在 STA 已关联时有效，否则返回 `ATBM_WIFI_ERR_NOT_READY`。
- `phy_rate` 在底层暂时无法给出时返回 `0`，不伪造速率。
- IP 信息按照当前模式选择接口：STA/P2P GO 使用 `if_id 0`，AP 使用 `if_id 1`。
- 客户端查询和踢除仅允许 AP/P2P GO；AP 使用 `if_id 1`，P2P GO 使用 `if_id 0`。
- `client_count` 返回实际复制到调用者缓冲区的数量；客户端多于
  `max_clients` 时安全截断。
- 客户端 RSSI 无有效值时返回驱动约定的未知值，不影响 MAC 列表返回。

现有 `atbm_wifi_stop_sta()` 继续承担 STA 断开并关闭接口的职责。本次不新增一个名字
不同但行为相同的 `WIFI_Disconnect()` 包装。

## P2P GO 启动和切换

`atbm_wifi_start_p2p()` 保持以下顺序：

1. 校验设备名、SSID、WPA2 密码和信道；
2. 如当前为 STA/AP，先停止当前接口和对应 DHCP 服务，但保留已初始化的 SDIO、固件
   和 ATBM 硬件上下文；
3. 调用 `atbm_wifi_p2p_user_config()`；
4. 调用 `atbm_wifi_p2p_start()`；
5. 调用 `atbm_wifi_p2p_go_start()`；
6. 在 `if_id 0` 配置 GO 地址并启动 GP 的 LwIP DHCP Server；
7. 将公共状态更新为 `P2P_GO/STARTED` 并发送 `ATBM_WIFI_EVENT_STARTED`。

P2P GO 切回 STA 时依次停止 DHCP Server、GO 和 P2P 任务，关闭 `if_id 0`，随后由
`atbm_wifi_start_sta()` 重新拉起 STA 接口。整个过程不调用
`atbm_wifi_hw_deinit()`，因此不重新初始化 SDIO，也不重新下载固件，与 ATM630 的热
切换原则一致。

P2P 本次只承诺自主 GO 模式，不新增 P2P Client、发现、协商或邀请等尚未经过 GP
平台验证的公共接口。

## P2P 底层修正

在私有驱动 API 中修正以下确定性问题：

- `atbm_wifi_p2p_start()` 必须先判断可用 VIF 是否为空，再读取 `priv->if_id`；
- `atbm_wifi_p2p_go_start()` 返回 `atbm_p2p_go_start()` 的真实结果，不再无条件成功；
- `atbm_wifi_p2p_user_config()` 对重复配置复用或释放旧配置，避免每次启动泄漏；
- 对设备名、SSID、密码长度做驱动侧边界保护，即使私有接口被直接调用也不能越界；
- 任一步骤失败时按 DHCP、GO、P2P 任务、接口的反顺序回滚，并保留原始失败结果；
- 停止完成后清理 P2P 用户配置，避免下一次启动继承旧参数。

## AP/P2P 客户端管理

复用驱动的 `WLAN_MACLIST`、硬连接表和 `atbmwifi_ap_deauth()`，在私有驱动 API 中
增加按 `if_id` 查询客户端、查询客户端 RSSI、断开指定客户端的安全包装。

同时修正现有 `atbm_wifi_get_associated_client_list_vif()`：当前代码先清空缓冲区，
导致调用者传入的容量 `count` 被清零，最终始终得不到客户端。新实现先保存容量，清空
输出后恢复容量，再读取硬连接 MAC；并校验缓冲区大小、VIF、模式和 MAC 参数。

公共层不得直接访问 `struct atbmwifi_vif` 或 `link_id_db`，避免应用接口绑定驱动内部
布局。

## 状态、事件和并发

- 所有新增公共接口沿用 `g_wifi.lock` 串行化模式切换和查询；
- 驱动事件继续映射为现有公共事件，不新增重复事件编号；
- P2P GO 客户端接入/离开分别上报
  `ATBM_WIFI_EVENT_CLIENT_CONNECTED/DISCONNECTED`；
- 启动失败时公共状态为 `ERROR`，但内部已完成本次部分启动资源的回滚；
- 回调在公共锁释放后触发，避免应用回调再次调用 WiFi API 时死锁；
- 密码不写入调试日志。

## 文件范围

预计修改：

- `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi.h`
- `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi_driver_api.h`
- `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_interface.c`
- `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_wifi_driver_api.c`
- `Project/wifi_demo/atbm_wifi_probe_demo.c`
- `tools/wireless_lib/tests/test_atbm_wifi_interface.ps1`
- `tools/wireless_lib/tests/test_atbm_public_api.ps1`

不修改 GP SDIO 控制器专用 `drvl1_sdc_*`/IRQ 实现，不引入 ATM630 SoC 适配代码。

## 验证

本地先完成：

1. 公共头文件自包含和稳定枚举值检查；
2. 新增类型、函数和私有包装的静态契约测试；
3. ATBM WiFi 静态库构建及新增符号检查；
4. 检查 P2P 各失败分支均有对应清理；
5. 检查应用示例只包含公共 `atbm_wifi.h`。

主工程由用户在 G+IDE 编译。上板至少验证：

1. 冷启动 P2P GO，手机发现、连接、获取 IP、正常收发；
2. P2P 客户端接入/离开事件、列表、RSSI 和指定断开；
3. STA → P2P GO → STA 连续切换，不重新下载固件且无崩溃；
4. 重复启动/停止 P2P，不出现配置残留、内存持续减少或 DHCP Server 重复实例；
5. AP 模式客户端列表和指定断开功能不回归。
