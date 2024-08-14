cmake_minimum_required(VERSION 3.10)
project(MprpcConfigTest)

set(CMAKE_CXX_STANDARD 17)

# 设置头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 设置源文件
set(SOURCES
    ${PROJECT_SOURCE_DIR}/mprpchannel.cpp
)

# 添加测试可执行文件
add_executable(test_mprpc_config ${PROJECT_SOURCE_DIR}/test_mprpc_config.cpp ${SOURCES})

# 查找并链接 Protobuf 库（如果需要）
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
target_link_libraries(test_mprpc_config ${PROTOBUF_LIBRARIES})