cmake_minimum_required(VERSION 3.24)

project(gpcv2178_lvgl_pc_simulator LANGUAGES C)

set(SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(LVGL_ROOT "${SDK_ROOT}/Project/LVGL/lvgl")
set(LV_CONF_DIR "${SDK_ROOT}/Project/LVGL")
set(SDL2_ROOT "${CMAKE_CURRENT_LIST_DIR}/third_party/SDL2-2.32.10")

if(NOT EXISTS "${LVGL_ROOT}/lvgl.h")
    message(FATAL_ERROR "SDK LVGL source was not found: ${LVGL_ROOT}")
endif()

if(NOT EXISTS "${SDL2_ROOT}/include/SDL.h")
    message(FATAL_ERROR "SDL2 development package was not found: ${SDL2_ROOT}")
endif()

# The SDK LVGL tree is fixed for this simulator.  Do not use CONFIGURE_DEPENDS:
# with the Visual Studio generator it adds a ZERO_CHECK VerifyGlobs pre-build
# step, which is unreliable in the current endpoint-security environment.
file(GLOB_RECURSE LVGL_SOURCES "${LVGL_ROOT}/src/*.c")

add_library(SDL2::SDL2 SHARED IMPORTED GLOBAL)
set_target_properties(SDL2::SDL2 PROPERTIES
    IMPORTED_IMPLIB "${SDL2_ROOT}/lib/x64/SDL2.lib"
    IMPORTED_LOCATION "${SDL2_ROOT}/lib/x64/SDL2.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_ROOT}/include")

add_library(SDL2::SDL2main STATIC IMPORTED GLOBAL)
set_target_properties(SDL2::SDL2main PROPERTIES
    IMPORTED_LOCATION "${SDL2_ROOT}/lib/x64/SDL2main.lib")

add_executable(lvgl_pc_simulator WIN32
    src/main.c
    src/ui_dashboard.c
    ${LVGL_SOURCES})

target_compile_features(lvgl_pc_simulator PRIVATE c_std_11)
set_target_properties(lvgl_pc_simulator PROPERTIES OUTPUT_NAME "lvgl_pc_simulator_png")
target_compile_definitions(lvgl_pc_simulator PRIVATE
    LV_CONF_INCLUDE_SIMPLE=1
    UI_ASSET_DIR=\"${CMAKE_CURRENT_BINARY_DIR}/ui/\")
target_include_directories(lvgl_pc_simulator PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/config"
    "${LVGL_ROOT}"
    "${LV_CONF_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/assets")
target_link_libraries(lvgl_pc_simulator PRIVATE SDL2::SDL2main SDL2::SDL2)

# The literal percent sign in this source filename is interpreted by cmd.exe
# when used in a POST_BUILD command.  Copy it during CMake configuration instead.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ui")
file(COPY_FILE
    "${CMAKE_CURRENT_LIST_DIR}/assets/04_文字图层/Battery_电量仪表/087_83%.png"
    "${CMAKE_CURRENT_BINARY_DIR}/ui/b87.png"
    ONLY_IF_DIFFERENT)

add_custom_command(TARGET lvgl_pc_simulator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:SDL2::SDL2>"
        "$<TARGET_FILE_DIR:lvgl_pc_simulator>"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/ui"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/02_组合控件/001_BG_背景.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/001.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/02_组合控件/003_Frame_面板框架.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/003.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/02_组合控件/015_Road Map_导航地图.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/015.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/02_组合控件/021_Player_音乐播放器.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/021.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/02_组合控件/031_Icons_底部图标.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/031.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/03_无文字控件/039_Grade_温度仪表_无文字.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/039.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/02_组合控件/049_Warning_警告提示.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/049.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/02_组合控件/052_Descriptions_导航文字.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/052.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/03_无文字控件/059_Diagrams_主仪表圆环_无文字.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/059.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/02_组合控件/067_Gear_挡位.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/067.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/02_组合控件/070_Time_时间与网络.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/070.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/02_组合控件/075_Weather_天气日期.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/075.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/02_组合控件/079_Battery_电量仪表.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/079.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/02_组合控件/088_Number_加减调节.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/088.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/03_无文字控件/095_Speed_速度显示_无文字.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/095.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/02_组合控件/100_User_用户信息.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/100.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/02_组合控件/105_Parking_泊车雷达.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/105.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Icons_底部图标/032_car.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/i32.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/大灯/worker-out.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/h33off.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/大灯/bigdeng.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/h33on.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/语音/voice0.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/v0.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/语音/voice1.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/v1.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Icons_底部图标/034_call.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/i34.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Icons_底部图标/035_plugin.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/i35.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Icons_底部图标/036_music.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/i36.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Icons_底部图标/037_speed.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/i37.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Icons_底部图标/038_luck.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/i38.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Grade_温度仪表/040_Ellipse 2.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/t40.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Grade_温度仪表/041_Ellipse 1.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/t41.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Grade_温度仪表/042_dashed line 3.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/t42.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Grade_温度仪表/043_dashed line 4.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/t43.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Grade_温度仪表/045_triangle 2.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/t45.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Grade_温度仪表/046_temperature.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/t46.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Grade_温度仪表/047_temperature circle.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/t47.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/04_文字图层/Grade_温度仪表/048_54.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/t48.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Diagrams_主仪表圆环/060_circle 1.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/m60.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Diagrams_主仪表圆环/061_light 1.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/m61.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Diagrams_主仪表圆环/062_light 2.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/m62.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Diagrams_主仪表圆环/064_line.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/m64.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/04_文字图层/Diagrams_主仪表圆环/065_23058.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/m65.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/04_文字图层/Diagrams_主仪表圆环/066_km.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/m66.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/04_文字图层/Speed_速度显示/099_km_h.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/s99.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Warning_警告提示/050_warning.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/w50.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/04_文字图层/Warning_警告提示/051_Dynamic Mode_  In dynamic mode, the driver will be restricted f.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/w51.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Battery_电量仪表/080_circle 2.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/b80.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Battery_电量仪表/081_circle 3.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/b81.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Battery_电量仪表/082_dashed line 1.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/b82.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Battery_电量仪表/083_dashed line 2.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/b83.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Battery_电量仪表/084_triangle 1.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/b84.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Battery_电量仪表/085_bettery  1.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/b85.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/assets/01_独立元素/Battery_电量仪表/086_bettery 2.png" "${CMAKE_CURRENT_BINARY_DIR}/ui/b86.png")
