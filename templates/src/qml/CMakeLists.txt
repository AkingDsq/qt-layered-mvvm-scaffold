# View 层：QML 源码编译为一个静态库，并注册为 QML 模块。
# qt_add_qml_module 会额外生成插件目标 <target>plugin（即 {{QML_TARGET}}plugin），
# app 层必须链接它，QML 类型与资源才会进入可执行文件。
#
# URI 与 target 是两件事：URI 是 QML 里 import 的模块名，target 是 CMake 目标名。
# 这里既不要写 OUTPUT_DIRECTORY（输出位置由顶层 QT_QML_OUTPUT_DIRECTORY 统一决定），
# 也不要写 RESOURCE_PREFIX（资源前缀由 QTP0001 策略决定，见顶层 qt_standard_project_setup）。
qt_add_library({{QML_TARGET}} STATIC)

qt_add_qml_module({{QML_TARGET}}
    URI "{{QML_URI}}"
    VERSION 1.0
    QML_FILES
        Main.qml
)

target_link_libraries({{QML_TARGET}} PRIVATE
    Qt6::Quick
    Qt6::Qml
)
