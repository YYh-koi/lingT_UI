# GPCV2178 LVGL PC Simulator

This project renders a 1024 x 600 LVGL 8.3.11 screen with SDL2 on Windows.
It reuses the SDK LVGL source at `Project/LVGL/lvgl`; it does not use vcpkg or Ninja.

## VS Code

1. Open this folder in VS Code.
2. Install the Microsoft **C/C++** and **CMake Tools** extensions.
3. Run **CMake: Select Configure Preset** and choose `MSVC x64 (Visual Studio generator)`.
4. Run **CMake: Build** with the `debug` preset.
5. Run `build/msvc-x64/Debug/lvgl_pc_simulator.exe`.

The project intentionally uses the Visual Studio generator, not Ninja, because Ninja is currently being interfered with by the local endpoint security environment.

## UI implementation

The simulator enables its own PNG decoder and loads the 17 layered PNG files
from `assets/02_组合控件` at runtime. Their positions and draw order follow
`assets/素材清单.json`; no image C array is compiled. It exposes `ui_dashboard_set_track()` and
`ui_dashboard_set_call()` for the next pass, which will use the supplied
no-text source layers for dynamic Bluetooth values.

Board-specific drivers and FreeRTOS calls stay out of this UI module.

## One-time NAND UI installer

The normal firmware reads LVGL v8 image binaries from the NAND application area
as `U:/UI/<name>.bin`; it never writes that area. To provision it from an SD card:

1. Generate the required `.bin` files from the same flattened 1024×600 PNG set
   used by the PC simulator. The included converter writes LVGL v8 binary files
   for this board's RGB565, little-endian configuration and preserves alpha:

   ```powershell
   powershell -ExecutionPolicy Bypass -File tools/lvgl_pc_simulator/convert_lvgl_bin.ps1
   ```

   Its output is `tools/lvgl_pc_simulator/ui_bin`. Copy the flat, ASCII-named
   files to the FAT32 SD-card folder `UI` (for example `UI/001.bin`,
   `UI/080.bin`).
2. Generate the versioned integrity manifest. Increment `--version` for every
   asset release:

   ```powershell
   python tools/lvgl_pc_simulator/make_nand_ui_manifest.py --ui-dir E:\UI --version 1
   ```

3. Make a **temporary installer firmware** by changing both
   `NAND_APP_WRITE_EN` and `LVGL_UI_NAND_INSTALLER` to `1` in
   `Project/GPCV2178_Turnkey_SDK/inc/gplib_cfg.h`; build it with the board IDE.
4. Boot that installer firmware once with the SD card inserted. It copies every
   resource, verifies the NAND CRC after each write, then writes `manifest.bin`
   last as its successful-install record. An equal or older manifest version is
   skipped, so normal reboots do not repeatedly program NAND.
5. Restore both macros to `0`, build and flash the normal firmware, and remove
   the SD card. The normal firmware is NAND read-only again.

Do not enable the two write macros in a production firmware. The installer does
not format NAND or delete unrelated files; it requires a mountable NAND
application area and a valid `SD:/UI/manifest.bin`.
