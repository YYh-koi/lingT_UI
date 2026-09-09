# ATBM WiFi 对外接口封装设计

## 目标

参考
`D:\Project\Outside\ZhengXin\amt630hV100-sdk-beta-bc4661_zhengxin\amt630hv100-freertos\lib\wlan\atbm_wifi6\iot_wifi_atbm6165.c`
的封装层次，为 GPCV2178 ATBM6165 静态库增加稳定的业务级接口。

客户通过 `atbm_wifi_interface.h` 使用 AP、STA 和 P2P，不直接依赖
`atbm_wifi_on()`、`atbm_wifi_ap_create()`、`atbm_wifi_p2p_start()` 等原厂内部接口。

客户交付物仍然只包含头文件和静态库，不包含 WiFi 实现源码。

## 文件组织

内部 SDK：

```text
gplib/liba_wifi_src/atbm/atbm6165/
├─ api/
│  ├─ atbm_wifi_interface.h
│  └─ atbm_wifi_types.h
├─ hal/
│  └─ atbm_wifi_interface.c
└─ wlan_lib/
   ├─ libatbm_wifi.a
   └─ libbtstack.a
```

`atbm_wifi_interface.c` 编入 `libatbm_wifi.a`。客户包不复制该源文件。

客户包中的正式 WiFi 接口只依赖 `atbm_wifi_interface.h` 和
`atbm_wifi_types.h`。现有原厂头文件可以保留在内部源码树中，但不再作为推荐的
客户调用入口。

## 公共数据类型

公共头文件不得包含 ATBM 私有 HAL、WPA、SDIO 或网络设备头文件。

```c
typedef enum {
    ATBM_WIFI_SECURITY_OPEN = 0,
    ATBM_WIFI_SECURITY_WEP,
    ATBM_WIFI_SECURITY_WPA_PSK,
    ATBM_WIFI_SECURITY_WPA2_PSK,
    ATBM_WIFI_SECURITY_WPA_WPA2_PSK
} atbm_wifi_security_t;

typedef enum {
    ATBM_WIFI_MODE_NONE = 0,
    ATBM_WIFI_MODE_STA,
    ATBM_WIFI_MODE_AP,
    ATBM_WIFI_MODE_P2P_GO
} atbm_wifi_interface_mode_t;

typedef enum {
    ATBM_WIFI_STATE_STOPPED = 0,
    ATBM_WIFI_STATE_STARTING,
    ATBM_WIFI_STATE_STARTED,
    ATBM_WIFI_STATE_CONNECTED,
    ATBM_WIFI_STATE_ERROR
} atbm_wifi_state_t;

typedef struct {
    const char *ssid;
    const char *password;
    int channel;
    atbm_wifi_security_t security;
    int hidden;
} atbm_wifi_ap_config_t;

typedef struct {
    const char *ssid;
    const char *password;
    const unsigned char *bssid;
    atbm_wifi_security_t security;
} atbm_wifi_sta_config_t;

typedef struct {
    const char *device_name;
    const char *ssid;
    const char *password;
    int channel;
} atbm_wifi_p2p_config_t;

typedef struct {
    unsigned char bssid[6];
    char ssid[33];
    int channel;
    int rssi;
    atbm_wifi_security_t security;
} atbm_wifi_scan_result_t;

typedef struct {
    atbm_wifi_interface_mode_t mode;
    atbm_wifi_state_t state;
} atbm_wifi_status_t;

typedef enum {
    ATBM_WIFI_EVENT_STARTED = 0,
    ATBM_WIFI_EVENT_STOPPED,
    ATBM_WIFI_EVENT_STA_CONNECTED,
    ATBM_WIFI_EVENT_STA_DISCONNECTED,
    ATBM_WIFI_EVENT_CLIENT_CONNECTED,
    ATBM_WIFI_EVENT_CLIENT_DISCONNECTED,
    ATBM_WIFI_EVENT_SCAN_DONE,
    ATBM_WIFI_EVENT_ERROR
} atbm_wifi_event_t;

typedef struct {
    atbm_wifi_interface_mode_t mode;
    atbm_wifi_event_t event;
    unsigned char mac[6];
    int error;
} atbm_wifi_event_info_t;

typedef void (*atbm_wifi_event_callback_t)(
    const atbm_wifi_event_info_t *event,
    void *user_data);

typedef enum {
    ATBM_WIFI_OK = 0,
    ATBM_WIFI_ERR_INVALID_ARG = -1,
    ATBM_WIFI_ERR_INVALID_STATE = -2,
    ATBM_WIFI_ERR_PLATFORM_INIT = -3,
    ATBM_WIFI_ERR_DRIVER_INIT = -4,
    ATBM_WIFI_ERR_START_FAILED = -5,
    ATBM_WIFI_ERR_NETWORK_FAILED = -6,
    ATBM_WIFI_ERR_NO_MEMORY = -7,
    ATBM_WIFI_ERR_NOT_READY = -8
} atbm_wifi_error_t;
```

SSID 最大长度为 32 字节。AP/STA 的 WPA/WPA2 密码长度为 8 至 63 字节。
P2P 以 GO 模式启动，行为与参考文件的 `start_p2p()` 一致。

## 公共函数

```c
int atbm_wifi_start_ap(const atbm_wifi_ap_config_t *config);
int atbm_wifi_stop_ap(void);

int atbm_wifi_start_sta(const atbm_wifi_sta_config_t *config);
int atbm_wifi_stop_sta(void);

int atbm_wifi_start_p2p(const atbm_wifi_p2p_config_t *config);
int atbm_wifi_stop_p2p(void);

int atbm_wifi_scan(atbm_wifi_scan_result_t *results,
                   unsigned int max_results,
                   unsigned int *result_count);

int atbm_wifi_get_status(atbm_wifi_status_t *status);
int atbm_wifi_get_mac(unsigned char mac[6]);

int atbm_wifi_register_event_callback(
    atbm_wifi_event_callback_t callback,
    void *user_data);

int atbm_wifi_deinit(void);
```

所有函数成功返回 `0`，失败返回公共头文件中定义的负数错误码。客户不需要理解
ATBM 内部错误码。

## 启动流程

### 公共初始化

三个 `start` 接口均执行一次性平台初始化：

1. 打开 WiFi 电源；
2. 配置并初始化 Generalplus SDIO；
3. 初始化 LwIP；
4. 初始化 ATBM SDIO 驱动和固件；
5. 创建内部状态锁并记录当前模式。

重复调用当前已启动模式时直接返回成功。请求启动不同模式时，先停止当前模式，
再启动新模式。模式切换和状态更新由内部锁串行化。

### AP

`atbm_wifi_start_ap()`：

1. 校验 SSID、密码、信道和安全类型；
2. 调用 `atbm_wifi_on(ATBM_WIFI_AP_MODE)`；
3. 调用 `atbm_wifi_ap_create()`；
4. 配置 AP 的 LwIP 网络接口；
5. 启动 DHCP Server；
6. 更新状态并报告事件。

`atbm_wifi_stop_ap()` 停止 DHCP Server、关闭 AP 接口并清理 AP 状态。

### STA

`atbm_wifi_start_sta()`：

1. 校验 SSID、密码、安全类型和可选 BSSID；
2. 调用 `atbm_wifi_on(ATBM_WIFI_STA_MODE)`；
3. 调用 `atbm_wifi_sta_join_ap()`；
4. 由现有 LwIP 适配层处理链路启用和 DHCP Client；
5. 更新状态并报告连接事件。

启动 STA 不强制执行扫描。客户需要扫描时显式调用 `atbm_wifi_scan()`。

`atbm_wifi_stop_sta()` 断开 AP、关闭 STA 接口并清理 STA 状态。

### P2P

`atbm_wifi_start_p2p()` 固定启动 P2P GO：

1. 校验设备名、SSID、密码和信道；
2. 调用 `atbm_wifi_p2p_user_config()`；
3. 调用 `atbm_wifi_p2p_start()`；
4. 调用 `atbm_wifi_p2p_go_start()`；
5. 配置实际的 P2P GO 网络接口（当前驱动使用 if_id 0）和 DHCP Server；
6. 更新状态并报告事件。

`atbm_wifi_stop_p2p()` 依次停止 GO、停止 P2P、关闭 if_id 0 和 DHCP Server。

## 扫描和查询

`atbm_wifi_scan()` 隐藏原厂可变长度扫描缓冲区。实现内部申请原始缓冲区，调用
`atbm_wifi_scan_network()`，再把结果转换为固定大小的
`atbm_wifi_scan_result_t` 数组。

当实际结果多于 `max_results` 时，只复制前 `max_results` 项，并通过
`result_count` 返回复制数量。

`atbm_wifi_get_mac()` 始终返回 6 字节二进制 MAC，不返回字符串。

`atbm_wifi_get_status()` 返回封装层维护的模式和状态，不向客户暴露私有
`struct atbmwifi_vif`。

## 事件

公共事件固定包括：

- WiFi 启动完成；
- WiFi 已停止；
- STA 已连接；
- STA 已断开；
- AP/P2P 客户端接入；
- AP/P2P 客户端离开；
- 扫描完成；
- 运行错误。

封装层将 `atbmwifi_event_register_cb()` 的原厂事件转换为公共事件。
回调参数中的 MAC、模式和状态使用公共结构体，回调不得暴露私有指针。

## 错误处理

公共错误码固定区分：

- 参数错误；
- 当前状态不允许该操作；
- 平台或 SDIO 初始化失败；
- 驱动或固件初始化失败；
- AP/STA/P2P 启动失败；
- 网络接口或 DHCP 配置失败；
- 内存不足；
- 未初始化或无有效连接。

任何启动步骤失败时，按相反顺序回滚本次已完成的步骤。状态最终设置为
`ATBM_WIFI_STATE_ERROR`，但仍允许调用对应停止接口或 `atbm_wifi_deinit()` 清理。

## 兼容与迁移

当前 `Project/wifi_demo/atbm_wifi_probe_demo.c` 改为调用新接口，不再直接包含
`atbm_wifi_driver_api.h`。主工程继续只链接 `libatbm_wifi.a`，不重复编译
`atbm_wifi_interface.c`。

原厂 API 继续保留在内部源码中，供库内部和调试使用；客户示例只展示
`atbm_wifi_interface.h`。

## 构建和交付

ATBM 独立 G+ 静态库工程增加 `atbm_wifi_interface.c`，输出仍为：

```text
gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a
```

客户包重新生成后必须满足：

- 不包含 ATBM WiFi 实现 `.c` 文件；
- 包含 `atbm_wifi_interface.h` 和它所需的公共类型头文件；
- `libatbm_wifi.a` 包含全部公共接口符号；
- 公共头文件可以仅使用客户 `api` 目录完成独立编译；
- 客户 SDK 可以仅使用头文件和 `.a` 完成链接。

## 验证

实施完成后执行：

1. 公共头文件独立编译测试；
2. AP、STA、P2P 参数校验和状态切换单元测试；
3. 静态库符号检查；
4. ATBM WiFi 独立工程 Debug 构建；
5. 客户包无无线源码检查；
6. 客户包最小编译和链接验证；
7. 更新现有文件修改 patch，并验证 patch 可应用。
