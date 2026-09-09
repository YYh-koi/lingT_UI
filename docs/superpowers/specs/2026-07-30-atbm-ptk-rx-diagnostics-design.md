# ATBM PTK 安装与 RX 解密诊断设计

## 背景

`Serial Debug 2026-07-30 104016.txt` 显示 SDIO 命令和数据传输稳定，AP 完成四次握手后持续收到 `WSM_STATUS_DECRYPTFAILURE(4)`，DHCP 只进行到 OFFER。当前 `wpa_common_install_ptk()` 忽略 `atbmwifi_set_key()` 的返回值，因此“4/4 ok”和“handshake_over”不能证明 PTK 已成功写入固件。

本工程的 `net/wpa/wpa_common.c` 与 `net/atbm_key.c` 和正芯 ATM630 两套 ATBM 源码逐字节一致。正芯版本具有相同的返回值遗漏，不能通过直接复制正芯文件修复。

## 方案比较

1. **最小错误回传与脱敏诊断（采用）**
   - 将 PTK 写入固件的返回值传回 hostapd/supplicant。
   - 在 AP pairwise key 路径校验 link_id 对应的 STA。
   - 输出 key 类型、索引、link_id、peer MAC、WSM 返回值，不输出密钥。
   - 将现有 RX 失败打印扩充为 status、link_id、帧控制、源 MAC、firmware flags。
   - 优点：改动小，能直接区分“密钥未写入”和“固件返回错误 link_id/peer”。

2. **直接替换为正芯 ATM630 文件**
   - 不采用。关键文件哈希相同，替换后行为不变，并可能覆盖本工程已有的 AP/SDIO 诊断修改。

3. **立即禁止 decrypt failure 触发 class3 deauth**
   - 暂不采用。它可以减少错误 deauth，但不能解决解密失败，也会隐藏 link_id 映射错误。待新日志证明 link_id 合法且仅为 decrypt failure 后再评估。

## 修改范围

- `gplib/liba_wifi_src/atbm/atbm6165/net/wpa/wpa_common.c`
  - 返回 `atbmwifi_set_key()` 的真实结果。
  - 仅记录 cipher、key index、link_id 和返回值。
- `gplib/liba_wifi_src/atbm/atbm6165/net/atbm_key.c`
  - AP pairwise key 安装前检查 `sta_priv`。
  - 记录 WSM key 元数据、peer MAC 和 `wsm_add_key()` 结果。
- `gplib/liba_wifi_src/atbm/atbm6165/hal/atbm_txrx.c`
  - 用一条脱敏 RX 错误记录替换信息不足的 `[RX] fail: N`。
  - 保持当前丢包和 class3 处理语义不变。

不修改 firmware、SDIO IRQ、DHCP、BTstack 和主工程链接配置。

## 数据流与预期结果

`hostapd_handshake_over()` / supplicant 调用 `wpa_common_install_ptk()`，后者调用 `atbmwifi_set_key()`，再由 `wsm_add_key()` 将密钥写入固件：

- WSM 写入失败：上层收到非零返回值，握手不会再被误报为成功，并打印 `[KEY] ... ret=<非零>`。
- WSM 写入成功但 RX 仍解密失败：新 `[RXERR]` 信息显示 firmware status、link_id、frame SA、flags/flags2，可判断是 link 映射、peer 不一致还是固件侧密钥状态异常。
- AP link_id 找不到对应 STA：安装立即失败并打印 `[KEY] AP pairwise STA missing`，避免空指针和错误 peerAddress。

## 验证

1. 静态回归检查先证明旧代码没有回传 PTK 安装错误，再验证修改后的控制流和脱敏日志字段。
2. 编译 `Project/WirelessLibraries/ATBM_WiFi_Library/ATBM_WiFi_Library.gpcmprj`，确认 `wpa_common.c`、`atbm_key.c`、`atbm_txrx.c` 均重新生成对象并写入 `libatbm_wifi.a`。
3. 检查主工程仍只链接更新后的 `libatbm_wifi.a`。
4. 硬件复测时关注 `[KEY]` 与 `[RXERR]`，不以单纯编译成功宣称无线问题已经解决。

