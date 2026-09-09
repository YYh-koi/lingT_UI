# ATBM WiFi 单头文件交付设计

## 目标

WiFi 部分面向客户只交付两个文件：

```text
WiFi/
├─ include/
│  └─ atbm_wifi.h
└─ lib/
   └─ libatbm_wifi.a
```

客户不需要获得或包含任何其他 ATBM 头文件。

## 公共头文件

`atbm_wifi.h` 是唯一客户接口文件，内容由现有
`atbm_wifi_interface.h` 和 `atbm_wifi_types.h` 合并而来。

它必须：

- 自包含所有公共整数类型、配置、状态、扫描结果、事件、回调和错误码；
- 声明 AP、STA、P2P GO、扫描、状态、MAC、事件注册和释放接口；
- 支持 C 和 C++ 调用；
- 不包含 `atbm_api.h`、`atbm_wifi_driver_api.h`、LwIP、FreeRTOS、SDIO
  或其他 ATBM 私有头文件。

客户只需：

```c
#include "atbm_wifi.h"
```

## 静态库

`libatbm_wifi.a` 继续包含：

- ATBM6165 WiFi 驱动；
- Generalplus 平台和 SDIO 适配；
- LwIP/DHCP 适配；
- AP、STA、P2P GO 和公共接口封装实现。

公共函数符号继续使用现有 `atbm_wifi_*` 名称，避免无意义的 ABI 变化。

## 内部源码

内部实现源文件改为包含 `atbm_wifi.h`。原厂头文件和私有类型仍可保留在完整
内部 SDK 中，供驱动实现使用，但不得进入 WiFi 客户交付目录。

内部独立 WiFi 库工程继续从完整源码构建，并输出：

```text
gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a
```

## 客户包规则

正式客户包的 WiFi 交付目录中必须且只能存在：

```text
WiFi/include/atbm_wifi.h
WiFi/lib/libatbm_wifi.a
```

不得交付：

- `atbm_wifi_interface.h`
- `atbm_wifi_types.h`
- `atbm_wifi_driver_api.h`
- `atbm_api.h`
- ATBM WiFi 实现 `.c` 文件

BTstack 不在本设计范围内，后续单独确定交付形式。

## 迁移

内部 demo 改为包含 `atbm_wifi.h`。现有公共函数名和结构体字段保持不变，因此
业务调用逻辑不需要改写。

现有文件的修改继续生成 patch；新增的 `atbm_wifi.h` 随交付文件直接提供。

## 验证

实施完成后必须验证：

1. `atbm_wifi.h` 仅使用自身包含路径即可编译；
2. 独立 WiFi 工程可以重新生成 `libatbm_wifi.a`；
3. 静态库包含全部 11 个公共接口符号；
4. demo 和客户最小工程只包含 `atbm_wifi.h` 即可编译、链接；
5. 客户 WiFi 交付目录严格只有一个头文件和一个 `.a`；
6. 已有文件修改 patch 可以应用。
