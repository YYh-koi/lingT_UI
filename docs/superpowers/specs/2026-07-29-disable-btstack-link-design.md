# 临时禁用 BTstack 链接设计

## 目的

在调试 WiFi/SDIO 期间临时排除 BTstack 对运行时行为、串口和中断的影响，并保留快速恢复能力。

## 方案比较

1. 注释主工程 Debug 目标中的 `libbtstack.a` 链接项：改动最小、容易恢复，采用。
2. 同时删除 BTstack 头文件路径和源码单元：影响范围大，当前没有必要。
3. 新增全局编译开关：适合长期维护，但会引入额外配置改造，当前不采用。

## 修改范围

仅修改：

`Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj`

将 Debug 目标 Linker 下的 `libbtstack.a` 条目改为 XML 注释。保留：

- BTstack 源码目录；
- BTstack include 路径；
- `libbtstack.a` 文件；
- 已经注释的 BTstack 源码单元。

Release 目标当前没有活动的 `libbtstack.a` 链接条目，不需要修改。

## 验证

- 验证工程 XML 可以正常解析；
- 确认工程中不再存在活动的 `libbtstack.a` Linker 条目；
- 执行一次 Debug 工程构建，确认没有未解析的 BTstack 符号；
- 不改动 WiFi/SDIO 驱动和库。

## 恢复

删除 `libbtstack.a` 条目外层的 XML 注释即可恢复。
