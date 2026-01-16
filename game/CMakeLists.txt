cmake_minimum_required(VERSION 3.16)
project(game VERSION 0.1.0 LANGUAGES C CXX)


set(SOLUTION_DIR ${PROJECT_SOURCE_DIR}/..)

# 自动递归查找所有源码文件
file(GLOB_RECURSE SRC_FILES
        ${PROJECT_SOURCE_DIR}/src/*.cpp
        ${PROJECT_SOURCE_DIR}/src/*.cc
        ${PROJECT_SOURCE_DIR}/src/*.c

        ${PROJECT_SOURCE_DIR}/code-gen/*.cpp
        ${PROJECT_SOURCE_DIR}/code-gen/*.cc
        ${PROJECT_SOURCE_DIR}/code-gen/*.c


        ${SOLUTION_DIR}/code-gen/proto-gen/*.cpp
        ${SOLUTION_DIR}/code-gen/proto-gen/*.cc
        ${SOLUTION_DIR}/code-gen/proto-gen/*.c
)
# 生成可执行文件
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/dos
        ${CODE_GEN}/proto-gen
)
# 链接操作

set(PROJECT_LINK_LIBS athena-discovery dal engine-lua)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_LINK_LIBS})