cmake_minimum_required(VERSION 3.21)
project({{PROJECT_NAME}} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 允许通过环境变量 QT_DIR 提示 Qt 安装位置（例如 C:/Qt/6.12.0/mingw_64）
if(NOT DEFINED CMAKE_PREFIX_PATH AND DEFINED ENV{QT_DIR})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{QT_DIR}")
    message(STATUS "Added QT_DIR to CMAKE_PREFIX_PATH: $ENV{QT_DIR}")
endif()

# 需要 Qt 6.5+（Main.qml 用到 Application.styleHints，Qt 6.5 引入）
find_package(Qt6 REQUIRED COMPONENTS Quick Qml Core Gui)

# REQUIRES 同时做两件事：声明最低 Qt 版本，并把该版本之前的 Qt CMake 策略置为 NEW
# —— 其中 QTP0001 决定 QML 模块的资源前缀 :/qt/qml/。不要自己写 qt_policy 去猜版本，
# 更不要用 QT_VERSION 判断：Qt 只提供 QT_VERSION_MAJOR/MINOR/PATCH，那个条件恒为假，
# 策略会静默失效（只有一句 "policy is not set" 的开发期警告）。
# 以后若往 qml/ 加子目录（需要 QTP0004 生成额外 qmldir），把这里提到 6.8。
qt_standard_project_setup(REQUIRES 6.5)

# qt_standard_project_setup 打开 MOC 与 UIC，但不含 RCC，所以资源编译要自己开。
set(CMAKE_AUTORCC ON)

# 所有 QML 模块统一输出到 <build>/qml/<target path>。
# 这样就不必在每个 qt_add_qml_module 里手写 OUTPUT_DIRECTORY，也就不会出现
# “OUTPUT_DIRECTORY 与 URI 推导出的 target path 不一致”的警告（qmllint 会失效）。
# 必须是绝对路径。
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml")

if(EMSCRIPTEN)
    # WebAssembly 目标
    message(STATUS "Building for WebAssembly")
else()
    # 桌面目标
    message(STATUS "Building for Desktop")
endif()

# 分层装配顺序：View(qml) -> Infrastructure -> Domain -> App(ViewModel + 组合根)
add_subdirectory(src)

# 测试与生产代码完全隔离，默认关闭；正式打包时保持 OFF
option(BUILD_TESTS "Build tests" OFF)
message(STATUS "BUILD_TESTS = ${BUILD_TESTS}")

if(BUILD_TESTS)
    enable_testing()
    include(CTest)
    add_subdirectory(tests)
endif()
