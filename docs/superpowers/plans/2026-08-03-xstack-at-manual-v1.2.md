# xStack AT 协议手册 V1.2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox syntax for tracking.

**Goal:** 将 xStack AT 协议手册改造成先展示 Command/Event 大全表、再展示详细条目的 V1.2 客户版，并修正品牌清理后不连贯的语句。

**Architecture:** 继续以 build_xstack_at_manual.py 中的 COMMANDS 和 EVENTS 为唯一内容源。新增 DOCX/PDF 总表渲染函数并调整章节顺序；总表和详细章节共用同一标题、状态与 ID，防止内容漂移。生成后使用独立验证脚本检查结构顺序、覆盖数量、禁用词和元数据。

**Tech Stack:** Python 3.12、python-docx、ReportLab、pypdf、Poppler。

---

### Task 1: 建立 V1.2 结构验收脚本

**Files:**
- Create: tmp/docs/xstack_at_manual/verify_xstack_at_manual_v1_2.py
- Test: output/docs/xStack_AT命令与事件完整协议手册_V1.2.docx
- Test: output/pdf/xStack_AT命令与事件完整协议手册_V1.2.pdf

- [ ] **Step 1: 编写失败优先的结构检查**

检查 V1.2 输出存在，并执行：

    assert text.index("Command 命令总表") < text.index("Command 详细说明")
    assert text.index("Event 事件总表") < text.index("Event 详细说明")
    assert len(set(re.findall(r"CMD-\d{3}", text))) == 79
    assert len(set(re.findall(r"EVT-\d{3}", text))) == 48
    assert "支持状态索引" not in text
    assert forbidden_word not in text.lower()

- [ ] **Step 2: 运行检查并确认失败**

Run:

    & $BundledPython tmp/docs/xstack_at_manual/verify_xstack_at_manual_v1_2.py

Expected: V1.2 文件尚不存在，脚本非零退出。

### Task 2: 调整版本与自然语言

**Files:**
- Modify: tmp/docs/xstack_at_manual/build_xstack_at_manual.py

- [ ] **Step 1: 将输出版本更新为 V1.2**

将 DOCX/PDF 文件名、封面、页脚和元数据版本全部改为 V1.2，保留 V1.1。

- [ ] **Step 2: 修订来源性措辞**

逐条执行：

    “参考协议默认值为 N” -> “缺省值为 N”
    “参考模块默认自动上电” -> “模块上电后默认自动启动蓝牙”
    “参考协议定义为……” -> 直接陈述命令行为
    “参考协议原文……” -> 从条目说明移入“命名与格式说明”
    “与参考协议的全量定义尚未完全对齐” -> 直接说明当前支持位和待实现位

- [ ] **Step 3: 扫描禁用词与生硬短语**

    rg -n -i "barrot|参考协议定义|参考协议原文|参考模块|与参考协议" tmp/docs/xstack_at_manual/build_xstack_at_manual.py

Expected: zero matches。

### Task 3: 在文档开头生成两个大全表

**Files:**
- Modify: tmp/docs/xstack_at_manual/build_xstack_at_manual.py

- [ ] **Step 1: 新增 DOCX 总表函数**

新增 add_overview_table(doc, heading, items, protocol_label)，生成 ID、Command/Event、中文功能简述、状态四列，使用显式 DXA 列宽、重复表头和状态色。

- [ ] **Step 2: 新增 PDF 总表函数**

新增 pdf_overview_flowables(heading, items, protocol_label, styles)，生成可跨页且 repeatRows=1 的四列表格。

- [ ] **Step 3: 调整章节顺序**

封面后依次放置 Command 命令总表、Event 事件总表、通用协议格式与状态说明、Command 详细说明、Event 详细说明、命名与格式说明。

- [ ] **Step 4: 删除文末重复索引**

移除 add_status_index(doc) 调用以及 PDF 的“支持状态索引”故事流。

### Task 4: 同步 Markdown 并生成交付文件

**Files:**
- Modify: docs/xstack_at_command_event_manual.md
- Modify: docs/xstack_at_protocol_inventory.md
- Create: output/docs/xStack_AT命令与事件完整协议手册_V1.2.docx
- Create: output/pdf/xStack_AT命令与事件完整协议手册_V1.2.pdf

- [ ] **Step 1: 运行生成器**

    & $BundledPython tmp/docs/xstack_at_manual/build_xstack_at_manual.py

Expected: commands=79 events=48 missing=0，并生成 V1.2 DOCX/PDF。

- [ ] **Step 2: 运行结构验收脚本**

    & $BundledPython tmp/docs/xstack_at_manual/verify_xstack_at_manual_v1_2.py

Expected: DOCX/PDF 的结构、数量、禁用词和版本检查全部通过。

### Task 5: 渲染和视觉核验

**Files:**
- Test: output/pdf/xStack_AT命令与事件完整协议手册_V1.2.pdf

- [ ] **Step 1: 使用 Poppler 渲染全部 PDF 页面**

    pdftoppm -png -r 110 output/pdf/xStack_AT命令与事件完整协议手册_V1.2.pdf tmp/docs/xstack_at_manual/pdf_pages_v1_2/page

- [ ] **Step 2: 检查所有页面**

检查封面、两个跨页总表、详细条目、页眉页脚、状态颜色和最后一页，确认无裁切、重叠、乱码或断裂表格。

- [ ] **Step 3: 最终机器校验**

重新打开 DOCX/PDF，确认 79 条 Command、48 类 Event、V1.2 版本、零禁用词以及 PDF 页数与渲染页数一致。
