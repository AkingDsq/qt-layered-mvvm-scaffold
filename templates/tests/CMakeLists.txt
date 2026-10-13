# 测试目录（本文件必须保持为合法 CMake：非注释行都会被当作命令解析）。
#
# 保持本文件存在，可以让根 CMakeLists.txt 里的 -DBUILD_TESTS=ON 永远配置成功。
# 需要真实测试时，取消下面的注释并补上源文件：
#
#   add_executable(smoke_test smoke_test.cpp)
#   target_link_libraries(smoke_test PRIVATE Qt6::Core domain)
#   add_test(NAME smoke COMMAND smoke_test)
#
# 测试目标只允许依赖 domain / infrastructure，不得依赖 app 或 QML 模块。
