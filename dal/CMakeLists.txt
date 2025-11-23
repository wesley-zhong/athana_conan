cmake_minimum_required(VERSION 3.16)
project(dal VERSION 0.1.0 LANGUAGES C CXX)

set(SOLUTION_DIR ../..)
find_package(protobuf CONFIG REQUIRED)

# 自动递归查找所有源码文件
file(GLOB_RECURSE SRC_FILES
        ${PROJECT_SOURCE_DIR}/*.cpp
        ${PROJECT_SOURCE_DIR}/*.cc
        ${PROJECT_SOURCE_DIR}/*.c
)

add_library(${PROJECT_NAME} STATIC
        ${SRC_FILES}
)

# 统一头文件路径，公开给下游（如 game）
target_include_directories(${PROJECT_NAME} PUBLIC
        ..
        ${3RD}
        ${PROJECT_SOURCE_DIR}/src/
)

# 链接依赖库
target_link_libraries(${PROJECT_NAME} PUBLIC
        core
        protobuf::libprotobuf
        mongo::mongocxx_static
        mongo::bsoncxx_static
        hiredis::hiredis
        libmysqlclient::libmysqlclient
)
