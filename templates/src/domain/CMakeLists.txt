# 领域层（MVVM 的 Model）：不依赖 View / ViewModel，只依赖基础设施。
# AppModel 是中性占位，换成你自己的领域模型时按实际文件名替换即可。
add_library(domain STATIC
    model/AppModel.h model/AppModel.cpp
)

target_include_directories(domain PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(domain PUBLIC
    Qt6::Core
    infrastructure
)
