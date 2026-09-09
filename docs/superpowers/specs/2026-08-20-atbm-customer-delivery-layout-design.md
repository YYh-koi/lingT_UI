# ATBM 客户交付目录与构建边界设计

## 目标

将 ATBM 使用的公共网络层放到 `gplib/liba_wifi_src/atbm/network`，形成与 8189 目录结构相近、但不依赖 `rtk` 的客户交付目录。网络层源码随 SDK 一起释放给客户，由客户主工程编译；ATBM6165 驱动本身仍以静态库和公开头文件交付。

## 最终目录

```text
gplib/liba_wifi_src/atbm/
├── network/
│   ├── dhcp/
│   ├── include/
│   ├── lwip_v2.1.3/
│   └── port/
└── atbm6165/
    ├── api/
    │   └── atbm_wifi.h
    └── wlan_lib/
        └── libatbm_wifi.a
```

现有目录：

```text
gplib/liba_wifi_src/atbm/atbm6165/platform/generalplus/network
```

迁移到：

```text
gplib/liba_wifi_src/atbm/network
```

迁移完成后不保留旧目录副本，避免两份网络源码发生配置漂移。

## 交付内容

客户侧 ATBM 集成包包含：

1. `gplib/liba_wifi_src/atbm/network` 网络层源码；
2. `libatbm_wifi.a` ATBM6165 驱动静态库；
3. `atbm_wifi.h` ATBM WiFi 公开接口头文件。

`network` 是 SDK 平台网络层，不属于 `libatbm_wifi.a` 的归档内容，但属于交付包的一部分。

## 构建边界

### ATBM WiFi 静态库

- 编译 ATBM6165 驱动、OS 适配和 SDIO 适配源码；
- 可以包含 `atbm/network` 中的头文件以完成驱动编译；
- 不编译 `atbm/network` 下的 lwIP、DHCP、FreeRTOS port 或 `atbm_lwip_port.c`；
- 输出 `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/libatbm_wifi.a`。

### ATBM 主工程

- 使用 `WIFI_VENDOR=3`；
- 链接 `libatbm_wifi.a`；
- 从 `gplib/liba_wifi_src/atbm/network` 编译 lwIP、DHCP、FreeRTOS port 和 ATBM lwIP 初始化适配；
- 不引用 `gplib/liba_wifi_src/rtk` 的路径、源码或库；
- 应用继续使用现有 `lwip/sockets.h`、`lwip/api.h` 和 `netconn_*()` 接口，不增加 `atbm_socket_*()` 封装。

### 8189FTV 主工程

- 保持独立工程配置；
- 使用 `WIFI_VENDOR=2`；
- 继续使用 `gplib/liba_wifi_src/rtk` 和 `lib8189FTV_WLAN.a`；
- 不引用 `libatbm_wifi.a`。

## 网络行为

- ATBM AP/P2P GO 默认 IPv4 地址继续使用 `192.168.42.1/24`；
- DHCP Server 继续由网络层启动；
- 网卡创建、lwIP 初始化、报文输入输出和 DHCP 行为保持现有实现；
- 本次只调整目录所有权与工程引用，不改变 WiFi、Socket 或 DHCP 对外行为。

## 验收标准

1. `atbm/network` 包含完整的 lwIP、DHCP、FreeRTOS port 和 ATBM lwIP port；
2. 旧的 `atbm6165/platform/generalplus/network` 不再存在；
3. ATBM WiFi 库工程不包含任何 network 源文件单元；
4. ATBM 主工程所有网络路径均指向 `gplib/liba_wifi_src/atbm/network`；
5. ATBM 工程元数据不存在 `liba_wifi_src/rtk` 引用；
6. 临时移走整个 `rtk` 目录后，ATBM WiFi 库和 ATBM 主工程仍能强制全量构建；
7. 恢复 `rtk` 后，独立 8189FTV 主工程能够构建；
8. 默认地址仍为 `192.168.42.1`；
9. 客户交付清单可明确列出 `network + libatbm_wifi.a + atbm_wifi.h`。

## 非目标

- 不把 network 源码归档进 `libatbm_wifi.a`；
- 不新增 ATBM Socket 包装接口；
- 不改变现有 AP、P2P、STA、DHCP 或 Socket 功能；
- 不删除或修改 8189FTV 所需的 RTK 网络目录。
