cmake_minimum_required(VERSION 3.16)
project(core VERSION 0.1.0 LANGUAGES C CXX)

# 自动递归查找所有源码文件
file(GLOB_RECURSE SRC_FILES
        ${PROJECT_SOURCE_DIR}/*.cpp
        ${PROJECT_SOURCE_DIR}/*.cc
        ${PROJECT_SOURCE_DIR}/*.c
)

add_library(${PROJECT_NAME} STATIC
        ${SRC_FILES}
)

# 第三方源码（joyent llhttp / http_parser）不改源文件，用 /wd 关掉其告警：
# llhttp.c   C4702(不可达) C4152(指针转换) C4244(截断)
# http_parser.cpp C4456(变量遮蔽)
if(MSVC)
    set_source_files_properties(${PROJECT_SOURCE_DIR}/utils/llhttp.c PROPERTIES
            COMPILE_OPTIONS "/wd4702;/wd4152;/wd4244")
    set_source_files_properties(${PROJECT_SOURCE_DIR}/utils/http_parser.cpp PROPERTIES
            COMPILE_OPTIONS "/wd4456")
endif()

# 统一头文件路径，公开给下游（如 gateway）
target_include_directories(${PROJECT_NAME} PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${3RD}
)

# 链接依赖库
target_link_libraries(${PROJECT_NAME} PUBLIC
        protobuf::protobuf
        spdlog::spdlog
        tomlplusplus::tomlplusplus
        rapidjson
)
