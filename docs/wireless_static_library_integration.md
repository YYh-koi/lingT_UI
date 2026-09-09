# WiFi / Bluetooth 静态库集成说明

## 客户交付布局

ATBM 无线模块按两个独立 ARM 静态库交付：

```text
gplib/liba_wifi_src/atbm/atbm6165/
├─ bt/
│  ├─ libxstack.a
│  └─ bt_xstack.h
└─ wlan_lib/
   ├─ libatbm_wifi.a
   └─ atbm_wifi.h
```

WiFi 部分给客户的文件只有 `atbm_wifi.h` 和 `libatbm_wifi.a`。客户业务代码只需要包含 `atbm_wifi.h`。该头文件提供 AP、STA、P2P GO、扫描、状态、MAC 和事件回调接口，并可独立包含，不依赖其他 ATBM、WPA、OS 或 SDIO 头文件。

Bluetooth 部分给客户的文件只有 `bt_xstack.h` 和 `libxstack.a`。客户业务代码包含 `bt_xstack.h` 后，可调用 `bt_xstack_init()` 和 `bt_xstack_send_at()`。

`atbm_wifi_types.h`、`atbm_wifi_driver_api.h` 和 `atbm_api.h` 只保留在内部完整 SDK，不进入客户包。客户包不包含 ATBM WiFi 或 BTstack 的实现 `.c` 文件。

两个库使用 Generalplus GCC 4.9.3 和主工程当前 Debug 配置构建，目标 ABI 为 Cortex-A7、NEON VFPv4、`softfp`。

## 客户接口示例

### 启动 AP

```c
#include "atbm_wifi.h"

static const atbm_wifi_ap_config_t ap_config = {
    .ssid = "wlan_ap_gp",
    .password = "12345678",
    .channel = 36,
    .security = ATBM_WIFI_SECURITY_WPA2_PSK
};

int result = atbm_wifi_start_ap(&ap_config);
```

### 启动 STA

```c
#include "atbm_wifi.h"

static const atbm_wifi_sta_config_t sta_config = {
    .ssid = "router_ssid",
    .password = "router_password",
    .security = ATBM_WIFI_SECURITY_WPA2_PSK
};

int result = atbm_wifi_start_sta(&sta_config);
```

### 启动 P2P GO

```c
#include "atbm_wifi.h"

static const atbm_wifi_p2p_config_t p2p_config = {
    .device_name = "GPCV2178-P2P",
    .ssid = "DIRECT-GPCV2178",
    .password = "12345678",
    .channel = 6
};

int result = atbm_wifi_start_p2p(&p2p_config);
```

切换模式前先调用当前模式对应的停止接口，或调用 `atbm_wifi_deinit()` 统一释放无线资源。

## 在 G+ IDE 中重新编译

WiFi 库工程：

```text
Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library.gpcmprj
```

BTstack 库工程：

```text
Project/WirelessLibraries/BTstack_Library/BTstack_Library.gpcmprj
```

分别在 G+ IDE 中打开工程，激活 `Debug` Target 后执行 Build。两个工程输出到：

```text
gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a
gplib/liba_wifi_src/atbm/atbm6165/bt/libxstack.a
```

库工程本身不进入客户包。

## 主工程链接

主工程：

```text
Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj
```

Debug Target 不再直接编译 ATBM WiFi 和 BTstack 实现源码，而是各链接一次：

```xml
<Add library="..\..\gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libatbm_wifi.a" />
<Add library="..\..\gplib\liba_wifi_src\atbm\atbm6165\bt\libxstack.a" />
```

若要修改未集成版本的工程文件，可使用：

```powershell
git apply --check patches/wireless_static_library_integration.patch
git apply patches/wireless_static_library_integration.patch
```

`patches/atbm_wlan_lib_layout.patch` 是旧版 `libbtstack.a` 布局迁移记录，
不适用于当前 `libxstack.a` 集成；当前工程只使用上面的
`wireless_static_library_integration.patch`。

`atbm_wifi.h` 和库中的接口实现源文件是新增文件，需要与补丁一起提供；补丁用于所有已有文件的修改。

## 生成客户 SDK

确保 `wlan_lib` 中的 WiFi 归档和 `bt` 中的 xStack 归档已经重新编译，然后在完整内部 SDK 根目录运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File tools/wireless_lib/package_wireless_release.ps1
```

默认输出：

```text
D:\Project\Platform\Generalplus\
GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm_customer
```

目标目录必须不存在或为空。脚本根据当前 Debug 工程递归收集产品配置需要的源码、头文件、工程、链接脚本及静态库。ATBM 目录强制只保留 `bt/{libxstack.a,bt_xstack.h}` 和 `wlan_lib/{libatbm_wifi.a,atbm_wifi.h}` 这四个交付文件；若客户工程仍依赖其他 ATBM 文件，打包会直接失败。

客户包中的无线库会执行 `--strip-debug` 和确定性 `ranlib -D`，随后生成 `SHA256SUMS.txt`。

## 验证客户包

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File tools/wireless_lib/verify_minimal_customer_build.ps1 `
  -CustomerRoot D:\Project\Platform\Generalplus\GPCV2178_Turnkey_SDK_SVN1864_20260603_atbm_customer
```

验证脚本只使用客户目录内容重新编译主工程并完成链接，同时检查 `atbm_wifi_start_ap`、`atbm_wifi_start_sta`、`atbm_wifi_start_p2p`、扫描/状态/MAC/事件/停止/释放接口及 BTstack 入口。
