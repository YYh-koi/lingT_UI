# AT frontend additions

- `PBAPSYNC` maps to `bt_xstack_phonebook_sync()`.
- `PBAPCANCEL` maps to `bt_xstack_phonebook_cancel()`.
- `PBAPAUTH` maps to `bt_xstack_phonebook_authenticate()`.
- `PBAPSTAT` maps to `bt_xstack_phonebook_get_state()`.

## BLE 与 GATT 命令映射

AT 版与 API 版复用同一套 BLE/GATT core dispatcher；AT worker 只负责解析命令和
格式化异步事件，不在 Run Loop 内执行应用输出。当前使用固定容量 RAM Bond（最多 4
个设备），重启后配对记录不保留。

- `LEADV` 对应 `bt_xstack_le_adv_configure()`、`bt_xstack_le_adv_start()` 和 `bt_xstack_le_adv_stop()`。
- `LESCAN` 对应 `bt_xstack_le_scan_start()` 和 `bt_xstack_le_scan_stop()`。
- `LECONN` 对应 `bt_xstack_le_connect()`。
- `LEDISC` 对应 `bt_xstack_le_disconnect()`。
- `LEPAIR` 对应 `bt_xstack_le_pair()`。
- `LECFM` 对应 `bt_xstack_le_pairing_confirm()`。
- `LEPASS` 对应 `bt_xstack_le_passkey_reply()`。
- `GATTSADD` 对应 GATT Server 添加 service。
- `GATTSC` 对应 GATT Server 添加 characteristic。
- `GATTSBUILD` 对应 GATT Server 构建属性数据库。
- `GATTSVAL` 对应 `bt_xstack_gatt_server_set_value()`。
- `GATTSNTF` 对应 `bt_xstack_gatt_server_notify()`。
- `GATTSIND` 对应 `bt_xstack_gatt_server_indicate()`。
- `GATTCDISC` 对应 `bt_xstack_gatt_client_discover_services()`。
- `GATTCC` 对应 `bt_xstack_gatt_client_discover_characteristics()`。
- `GATTCR` 对应 `bt_xstack_gatt_client_read()`。
- `GATTCW` 对应 `bt_xstack_gatt_client_write()`。
- `GATTCSUB` 对应 `bt_xstack_gatt_client_subscribe()`/`bt_xstack_gatt_client_unsubscribe()`。
