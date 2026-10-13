# 应用层：组合根 + ViewModel（MVVM 的 ViewModel）。
# 这一层是唯一允许同时看见 View、ViewModel、Model、Infrastructure 的地方。
qt_add_executable({{APP_TARGET}}
    WIN32
    main.cpp
    viewModel/ApplicationControllerViewModel.h viewModel/ApplicationControllerViewModel.cpp
)

target_sources({{APP_TARGET}} PRIVATE
    ../../resources/resources.qrc
)

target_include_directories({{APP_TARGET}} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries({{APP_TARGET}} PRIVATE
    Qt6::Quick
    Qt6::Qml
    {{QML_TARGET}}plugin
    domain
    infrastructure
)

# 静态 Qt（WebAssembly、iOS）下，QML 模块必须被链接进可执行文件；桌面版共享 Qt
# 是在运行时从 qml/ 目录动态加载，所以这条差异只在静态构建里才暴露。
#
# Qt 会自动做这件事：qt_add_executable() 的 finalizer 会调用
# qt6_import_qml_plugins()，用 qmlimportscanner 扫描后把静态 QML 插件链接进来。
# 但扫描范围只覆盖「本目标自己」的 QML 文件（-rootPath 是本目标的源码目录，
# -qrcFiles 取本目标的 _qt_generated_qrc_files）。本工程的 QML 位于独立的
# {{QML_TARGET}} 目标里，扫描结果会是 0 个导入，于是 QtQuick.Layouts /
# QtQuick.Controls.Basic 的插件不会被链接，运行时报 "module ... is not installed"。
# 下面这行把 QML 源码目录补进扫描范围（Qt 为此专门提供的目标属性）。
set_target_properties({{APP_TARGET}} PROPERTIES
    QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS "${CMAKE_SOURCE_DIR}/src/qml"
)
