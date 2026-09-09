# xStack AT 命令与事件完整协议手册实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 以 BARROT V1.2 为基线，生成完整收录 AT 命令和事件、使用中文说明并标注当前 xStack 支持状态的 DOCX/PDF 手册。

**Architecture:** 使用结构化协议清单作为单一内容来源，再由 Python 文档生成器创建 DOCX；DOCX 通过 LibreOffice 渲染生成最终 PDF。内容检查和版式检查相互独立，确保协议覆盖、中文说明和视觉质量均可验证。

**Tech Stack:** Markdown、Python、python-docx、OOXML、LibreOffice、Poppler、PowerShell。

---

### Task 1: 建立完整协议清单和支持状态

**Files:**
- Create: `docs/xstack_at_protocol_inventory.md`
- Reference: `D:/Project/Platform/ARK/XinBtech_atm630hv100_bt_wifi_20260526/doc/BARROT_AT_command&event_260310_V1.2.pdf`
- Reference: `btstack/at/btstack_at_internal.h`
- Reference: `btstack/at/btstack_at_parser.c`
- Reference: `btstack/at/btstack_at_backend.c`
- Reference: `btstack/at/btstack_at_protocol.c`

- [ ] **Step 1: 列出 BARROT 主协议命令**

按通用、HFP、A2DP/AVRCP、PBAP、SPP、GATT、HID 分组记录 71 条正式编号命令；另列出原文说明中的 8 条 iAP2/AAP 对应命令。

- [ ] **Step 2: 列出 BARROT 主协议事件**

按通用、HFP、A2DP/AVRCP、PBAP、SPP、GATT、HID、ANCS 分组记录 44 条正式编号事件；另列出原文说明中的 4 条 iAP2/AAP 对应事件。

- [ ] **Step 3: 标注支持状态**

逐项使用“已实现”“部分实现”“待实现”之一。当前 xStack 未提供底层 Profile 或 AT 路径的 PBAP、SPP、AVRCP 浏览/BIP、ANCS、iAP2/AAP 项必须标为“待实现”；存在参数或模式限制的项标为“部分实现”。

- [ ] **Step 4: 验证清单数量和状态完整性**

Run:

```powershell
rg -c '^\| CMD-' docs/xstack_at_protocol_inventory.md
rg -c '^\| EVT-' docs/xstack_at_protocol_inventory.md
rg -n '\|[[:space:]]*\|' docs/xstack_at_protocol_inventory.md
```

Expected: 命令行数为 79，事件行数为 48，不存在空状态或空中文说明字段。

### Task 2: 编写中文完整协议正文

**Files:**
- Create: `docs/xstack_at_command_event_manual.md`
- Reference: `docs/xstack_at_protocol_inventory.md`

- [ ] **Step 1: 编写协议总则和兼容性说明**

明确 `AT+<command>[=<parameter>...]\r\n`、`+<event>[=<parameter>...]\r\n`、`OK\r\n`、`ERROR\r\n` 和 `<FF>=0x0C`。

- [ ] **Step 2: 编写全部命令条目**

每条命令包含名称、中文用途、支持状态、格式、参数、响应、中文说明和示例。iAP2/AAP 对应命令按 SPP 语义展开，并注明其来源是 BARROT 文档中的映射说明。

- [ ] **Step 3: 编写全部事件条目**

每条事件包含名称、中文含义、支持状态、格式、参数、中文触发条件和示例。iAP2/AAP 对应事件按 SPP 语义展开。

- [ ] **Step 4: 编写勘误附录**

固定采用 `AT+HFPMCAL`、`AT+AVRCPCFG`、`+HFPVR`、`+TRACKINFO`、`AT+PBDISC`；HFP 状态范围按 `0~10`；所有原文矛盾均在附录说明。

- [ ] **Step 5: 运行正文结构检查**

Run:

```powershell
rg -c '^### CMD-' docs/xstack_at_command_event_manual.md
rg -c '^### EVT-' docs/xstack_at_command_event_manual.md
rg -n 'TBD|TODO|待补充|英文说明' docs/xstack_at_command_event_manual.md
```

Expected: 79 个命令条目、48 个事件条目，且没有占位内容。

### Task 3: 生成 DOCX

**Files:**
- Create: `tmp/docs/xstack_at_manual/build_xstack_at_manual.py`
- Create: `output/docs/xStack_AT命令与事件完整协议手册_V1.0.docx`
- Read: `docs/xstack_at_command_event_manual.md`

- [ ] **Step 1: 实现 DOCX 生成器**

使用 `compact_reference_guide` 预设，采用 A4 纵向命名覆盖。页面边距 18 mm，页眉/页脚 12.5 mm；中文字体为 Microsoft YaHei，西文字体为 Arial，协议文本为 Consolas；正文 10.5 pt，协议表格 9 pt。表格使用固定 DXA 宽度和显式单元格边距，不使用固定行高。

- [ ] **Step 2: 创建封面和导航结构**

使用 `editorial_cover` 开场结构，加入文档名、版本 V1.0、日期 2026-08-03、基线 BARROT V1.2、适用对象和状态图例。正文使用真实 Heading 1/2/3 样式。

- [ ] **Step 3: 生成命令和事件正文**

命令/事件条目使用标题、状态标签、固定宽度标签-内容表格、说明段落和等宽示例块；长说明不得塞入窄表格。

- [ ] **Step 4: 写入页眉页脚和页码字段**

页眉显示“xStack AT 命令与事件完整协议手册”，页脚显示“V1.0”和 PAGE 字段。

- [ ] **Step 5: 运行生成器和结构审计**

Run:

```powershell
& 'C:/Users/22148/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' `
  tmp/docs/xstack_at_manual/build_xstack_at_manual.py
```

Expected: DOCX 存在且非空；生成器报告 79 个命令、48 个事件、0 个缺失条目。

### Task 4: 渲染 DOCX 并生成 PDF

**Files:**
- Read: `output/docs/xStack_AT命令与事件完整协议手册_V1.0.docx`
- Create: `output/pdf/xStack_AT命令与事件完整协议手册_V1.0.pdf`
- Create: `tmp/docs/xstack_at_manual/rendered/page-*.png`

- [ ] **Step 1: 渲染 DOCX**

Run:

```powershell
& 'C:/Users/22148/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' `
  'C:/Users/22148/.codex/plugins/cache/openai-primary-runtime/documents/26.801.11242/skills/documents/render_docx.py' `
  'output/docs/xStack_AT命令与事件完整协议手册_V1.0.docx' `
  --output_dir 'tmp/docs/xstack_at_manual/rendered' --emit_pdf
```

Expected: 每页产生 PNG，且渲染目录中产生同名 PDF。

- [ ] **Step 2: 逐页视觉检查**

检查所有页面的中文字体、表格边界、换行、标题孤行、示例块、页眉页脚、页码、异常空白和重叠。发现问题后修改生成器并重新生成、重新渲染。

- [ ] **Step 3: 发布正式 PDF**

将最新渲染 PDF复制到 `output/pdf/xStack_AT命令与事件完整协议手册_V1.0.pdf`。

### Task 5: 最终内容与交付验证

**Files:**
- Verify: `output/docs/xStack_AT命令与事件完整协议手册_V1.0.docx`
- Verify: `output/pdf/xStack_AT命令与事件完整协议手册_V1.0.pdf`

- [ ] **Step 1: 提取 DOCX/PDF 文本做覆盖检查**

验证 79 个命令标识、48 个事件标识、三种支持状态、所有章节和勘误条目均存在。

- [ ] **Step 2: 检查文档元数据和页面**

确认两个交付文件版本均为 V1.0，PDF 为 A4、未加密、页数与 DOCX 渲染页数一致。

- [ ] **Step 3: 检查最终目录**

Run:

```powershell
Get-Item `
  'output/docs/xStack_AT命令与事件完整协议手册_V1.0.docx', `
  'output/pdf/xStack_AT命令与事件完整协议手册_V1.0.pdf' |
  Select-Object FullName,Length,LastWriteTime
```

Expected: 两个文件均存在且非空；最终回复只交付 DOCX 和 PDF，不交付临时 PNG。

本工作区不是 Git 仓库，因此本计划不包含 commit、分支或 worktree 清理步骤。
