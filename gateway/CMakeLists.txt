cmake_minimum_required(VERSION 3.16)
project(gateway VERSION 0.1.0 LANGUAGES C CXX)

# 源码
file(GLOB_RECURSE SRC_FILES
        ${PROJECT_SOURCE_DIR}/src/*.cpp
        ${PROJECT_SOURCE_DIR}/src/*.cc
        ${PROJECT_SOURCE_DIR}/src/*.c

        ${CODE_GEN}/proto-gen/*.cpp
        ${CODE_GEN}/proto-gen/*.cc
        ${CODE_GEN}/proto-gen/*.c
)

add_executable(${PROJECT_NAME}
        ${SRC_FILES}
)

# 统一头文件路径
target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${CODE_GEN}/proto-gen
)
# 链接操作
set(PROJECT_LINK_LIBS athena-discovery transport dal  engine-lua)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_LINK_LIBS})
