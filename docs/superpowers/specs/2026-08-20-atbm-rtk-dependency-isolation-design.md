# ATBM 与 RTK 构建依赖隔离设计

## 背景

当前 ATBM6165 WiFi 静态库工程和主工程虽然使用 `WIFI_VENDOR=3`，但构建配置仍引用 `gplib/liba_wifi_src/rtk`：

- ATBM 静态库从 RTK 目录取得 lwIP、DHCP 及若干 8189 兼容头文件。
- 主工程从 RTK 目录编译 lwIP、DHCP、8189 平台适配源码，并同时链接 8189 与 ATBM WiFi 库。
- 因此删除或改名 RTK 目录后，即使目标是 ATBM，构建仍会失败。

工程必须继续支持 8189FTV，但 ATBM 和 8189FTV 的构建依赖必须相互隔离。

## 目标

1. ATBM WiFi 静态库只依赖 ATBM 目录和平台公共目录。
2. ATBM 主工程不再引用、编译或链接 `gplib/liba_wifi_src/rtk` 下的任何文件。
3. 保留独立的 8189FTV 构建配置和原有 RTK 目录，不破坏以后切换回 8189FTV 的能力。
4. 保持当前 ATBM AP、P2P GO、DHCP、Socket、SDIO 及蓝牙相关行为不变。

## 非目标

- 不升级 lwIP 版本。
- 不重新实现 DHCP Server。
- 不重构应用层 WiFi 状态机或对外 WiFi API。
- 不把 ATBM 与 8189FTV 强行统一成一套厂商抽象层。

## 选定方案

采用“双构建配置 + ATBM 自包含网络栈副本”。已验证可用的 lwIP 2.1.3、FreeRTOS port、DHCP Server 和必要网络适配内容复制到 ATBM 平台目录；ATBM 工程只使用副本，8189FTV 工程继续使用原 RTK 内容。

相较于抽取两家共享的公共网络栈，该方案影响范围更小，也能满足 ATBM 目录自包含的交付要求。只复制头文件不能解决主工程仍从 RTK 编译网络源码的问题，因此不采用。

## 目录边界

ATBM 平台相关网络代码放在：

```text
gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/network/
├── dhcp/
│   ├── dhcps.c
│   └── dhcps.h
├── include/
│   ├── lwipopts.h
│   └── atbm_lwip_port.h
├── lwip_v2.1.3/
│   ├── ports/freertos/
│   └── src/
└── port/
    └── atbm_lwip_port.c
```

`atbm6165/net` 继续表示 802.11/WPA 协议实现，不放 TCP/IP 网络栈，避免语义混淆。

## 依赖处理

### lwIP

复制当前正式工程实际使用的 lwIP 2.1.3 源码、公共头文件和 FreeRTOS port，保持版本及配置不变。RTK 的 `ports/wifi/ethernetif.c` 不复制，因为 ATBM 已在 `atbm_os_skbuf.c` 中实现自己的 `netif_add()`、TX linkoutput 和 RX input 路径。ATBM 静态库工程和 ATBM 主工程的 include/source 路径统一改到 ATBM 平台目录。

### DHCP Server

复制当前主工程实际编译的 `rtk/network/dhcp/dhcps.c` 与 `dhcps.h`，保留现有租约逻辑、调试打印和已验证修复。ATBM 目录中已有但未参与正式构建的 DHCP 实现不直接替换生产版本。

### 网络适配接口

ATBM 源码不再包含 RTK 的 `lwip_netconf.h`。建立 ATBM 自有的 `atbm_lwip_port.h/.c`，只负责启动和关闭 lwIP TCP/IP 核心；网卡仍由 ATBM 驱动自身创建。调用点同步改用 ATBM 命名，避免以后再次通过同名头文件误引用 RTK 实现。

### RTK 兼容头文件

逐项消除 ATBM 对 `wifi_conf.h`、`wifi_ind.h`、`wifi_util.h`、`autoconf.h`、`wireless.h`、`wlan_intf.h`、`osdep_service.h` 等 8189 头文件的传递依赖。平台通用类型改用工程已有定义；ATBM 专用声明放回 ATBM 的 `include` 或 `platform/generalplus` 目录。

不复制整套 8189 API/OS/SDIO 头文件到 ATBM，以免形成改名后的隐性 RTK 副本。

## 构建配置

### ATBM 配置

现有正式 ATBM 工程继续使用 `WIFI_VENDOR=3`，并完成以下清理：

- 删除所有 `liba_wifi_src/rtk` include 路径。
- 删除所有 RTK/8189 源文件单元。
- 删除 `lib8189FTV_WLAN.a` 链接项。
- 增加 ATBM 自有 lwIP、DHCP 和网络适配源码/头文件。
- 保留 `libatbm_wifi.a` 及当前蓝牙库配置。

ATBM 静态库工程同步只保留 ATBM 自有 include 路径。

### 8189FTV 配置

从清理前的主工程配置保留一份独立的 8189FTV 工程文件，继续引用原 RTK 目录、`lib8189FTV_WLAN.a` 及其平台源码。该配置不参与“RTK 目录改名后构建 ATBM”的验收。

两套配置使用明确的工程文件名区分，禁止在 ATBM 工程中通过条件宏保留不可解析的 RTK 文件路径，因为 IDE 生成 makefile 时仍可能访问这些文件。

## 兼容性要求

- 应用层继续调用现有 `atbm_wifi_init()`、AP/P2P 模式接口及 Socket 接口。
- 当前 AP/P2P GO SSID、P2P device name、同步初始化流程保持不变。
- `ATBM_GENERALPLUS_SDIO` 的默认行为、SDIO host 锁、CMD53/DMA 等既有修复保持不变。
- DHCP Server 地址、租约行为与日志格式保持与当前正式构建一致。

## 验证与验收

1. 清理并构建 ATBM WiFi 静态库，确认生成 `libatbm_wifi.a`。
2. 清理并构建 ATBM 主工程，确认生成正式 ELF/BIN。
3. 扫描 ATBM 工程文件、生成 makefile、依赖文件和链接项，确认不存在 `liba_wifi_src/rtk`。
4. 将 `gplib/liba_wifi_src/rtk` 临时改名为同级备份名；在 `try/finally` 保护下重新执行前两项完整构建，然后恢复目录。
5. 恢复 RTK 目录后检查 8189FTV 独立工程配置仍完整，至少完成工程生成/依赖检查；若现有工具链支持，再执行完整 8189FTV 构建。

成功标准是：RTK 目录不存在时，ATBM WiFi 静态库和 ATBM 主工程均能从干净状态完成构建，且所有正式构建输入都来自 ATBM或平台公共目录。

## 风险控制

- 复制前记录源文件清单和哈希，避免漏文件或拿错 lwIP 版本。
- 优先修改 `.gpcmprj` 源工程文件，再重新生成或同步 makefile；不只修改生成文件。
- 保留用户当前工作区内所有无关修改，不覆盖现有 ATBM 功能修复。
- 临时改名 RTK 目录时使用解析后的绝对路径，并保证无论构建成功或失败都恢复原目录。
