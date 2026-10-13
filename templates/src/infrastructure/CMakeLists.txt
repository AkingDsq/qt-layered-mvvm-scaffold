# 基础设施层：只依赖 Qt 基础库，不依赖 domain / app / qml。
# 其他层通过它的 public 头文件目录使用日志门面（logging/LogApi.h）。
add_library(infrastructure STATIC
    logging/LogApi.h logging/LogApi.cpp
    logging/LoggerService.h logging/LoggerService.cpp
)

target_include_directories(infrastructure PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(infrastructure PUBLIC
    Qt6::Core
)
