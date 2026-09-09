# ATBM 网络地址初始化责任设计

## 目标

由 ATBM netif 默认初始化统一设置 IPv4 地址，网络服务启动阶段不再
覆盖地址。`atbm_wifi.h` 只保持应用工程需要的公共接口，不承担库内部
网络配置。

## 责任边界

两份 `atbm_wifi.h` 均保留，因为 `wlan_lib/atbm_wifi.h` 是应用工程交付头文件，
`api/atbm_wifi.h` 是源码工程的公共 API 头文件：

- `gplib/liba_wifi_src/atbm/atbm6165/api/atbm_wifi.h`
- `gplib/liba_wifi_src/atbm/atbm6165/wlan_lib/atbm_wifi.h`

两份公共头文件中都删除 `ATBM_WIFI_SERVER_*` 宏。它们不应暴露库内部
IP 设置，也不应因为同名头文件的搜索顺序而产生不同值。

默认地址归 `atbm_os_skbuf.c` 中的 netif 初始化所有：

- Server IP：`192.168.13.1`
- Netmask：`255.255.255.0`
- Gateway：`0.0.0.0`

在 `atbm_wifi_add_interfaces()` 创建 WLAN 和 P2P netif 时，
`atbm_wifi_netif_init()` 将这些值传给 `netif_add()`。因此 AP/P2P 网络服务
启动前 netif 已有地址。

## 实现

`atbm_wifi_network_server_start_locked()` 删除局部的 `ipaddr`、`netmask`、
`gateway`，删除三组 `IP4_ADDR()` 和 `netif_set_addr()`。

函数继续负责：

1. 校验 netif 存在；
2. 设置默认 netif，并将 link 和 interface 置为 up；
3. 使用 netif 当前地址启动 DHCP Server。

如果库内其他代码在启动网络服务前通过 `netif_set_addr()` 动态更改地址，
该地址将被保留，DHCP Server 使用更改后的 netif 地址和掩码。

## DHCP Server 行为

`dhcps_init()` 从 netif 读取服务器地址和掩码以计算地址池，并在 DHCP
Router 选项中下发 netif 的本地 IP。因此 netif 初始网关为 `0.0.0.0`
不会导致客户端收到 `0.0.0.0` 网关。

## 验证

更新最小契约测试，先观察它因现有覆盖行为而失败，再验证：

1. `atbm_wifi_network_server_start_locked()` 不再调用 `netif_set_addr()`；
2. 该函数仍启用 netif 并调用 `dhcps_init(netif)`；
3. `atbm_os_skbuf.c` 仍以 `192.168.13.1/24` 初始化 netif；
4. 两份 `atbm_wifi.h` 均不再含有 `ATBM_WIFI_SERVER_*` 宏；
5. 两份公共头文件除已知交付差异外保持接口一致；
6. 运行 ATBM 公共 API、P2P 和 network server 相关契约测试。

## 范围外事项

- 不删除任何一份 `atbm_wifi.h`；
- 不修改公开 API/ABI；
- 不修改 8189 Wi-Fi 配置；
- 不新增 IP 配置头文件或运行时配置 API；
- 不修改二维码等业务模块显示的地址。
