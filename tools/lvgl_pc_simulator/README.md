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
