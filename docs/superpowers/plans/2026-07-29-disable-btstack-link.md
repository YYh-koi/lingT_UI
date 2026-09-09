# Disable BTstack Link Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Temporarily remove BTstack from the Debug firmware link while preserving all BTstack sources, include paths, and archives.

**Architecture:** Modify only the Debug target Linker section of the Generalplus project XML. Wrap the active `libbtstack.a` library element in an XML comment, then validate that the project remains well-formed and no active BTstack linker element remains.

**Tech Stack:** Generalplus `.gpcmprj` XML project file, PowerShell XML validation, existing SDK build tooling.

---

### Task 1: Disable and verify the BTstack Debug link

**Files:**
- Modify: `Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj:203`

- [ ] **Step 1: Verify the active BTstack link exists**

Run:

```powershell
Select-String -LiteralPath 'Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj' -SimpleMatch '<Add library="..\..\gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libbtstack.a" />'
```

Expected: one active match in the Debug target Linker section.

- [ ] **Step 2: Comment the BTstack library element**

Replace:

```xml
<Add library="..\..\gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libbtstack.a" />
```

with:

```xml
<!-- Temporarily disabled while debugging WiFi/SDIO:
<Add library="..\..\gplib\liba_wifi_src\atbm\atbm6165\wlan_lib\libbtstack.a" />
-->
```

- [ ] **Step 3: Validate project XML and link state**

Run:

```powershell
$path = 'Project/GPCV2178_Turnkey_SDK/GPCV2178_turnkey_sdk_release.gpcmprj'
$xml = [xml](Get-Content -LiteralPath $path -Raw)
$active = $xml.SelectNodes('//Linker/Add[contains(@library,"libbtstack.a")]')
if ($active.Count -ne 0) { throw "Active libbtstack.a link remains" }
```

Expected: command exits successfully and reports no active BTstack linker element.

- [ ] **Step 4: Build the Debug target**

Run:

```powershell
$toolDir = 'D:\IDE\G+IDE_for_ARM_V1.2.0\toolchain\bin'
$makefile = 'output\wireless_libs\build\GPCV2178_no_btstack_Debug.mk'
New-Item -ItemType Directory -Force -Path (Split-Path $makefile) | Out-Null
& "$toolDir\gpcmprj2make.exe" --local -in 'Project\GPCV2178_Turnkey_SDK\GPCV2178_turnkey_sdk_release.gpcmprj' -cfg Debug -out $makefile -tp $toolDir -windows
& "$toolDir\make.exe" -f $makefile
```

Expected: the link succeeds without unresolved BTstack symbols, and the resulting firmware no longer contains linked BTstack code.

- [ ] **Step 5: Record source-control limitation**

This workspace has no `.git` metadata, so no Git commit is created. Preserve the XML comment as the reversible record of the temporary change.
