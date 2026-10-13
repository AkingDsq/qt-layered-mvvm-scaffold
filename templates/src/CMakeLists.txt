# 层间顺序即依赖顺序：先声明被依赖的层，再声明依赖方。
# qml        -> View 层（QML 模块）
# infrastructure -> 基础设施层（日志等横切能力）
# domain     -> 领域层（Model）
# app        -> 应用层（ViewModel + 组合根 main.cpp）
add_subdirectory(qml)
add_subdirectory(infrastructure)
add_subdirectory(domain)
add_subdirectory(app)
