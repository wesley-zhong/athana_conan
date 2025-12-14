cmake_minimum_required(VERSION 3.16)
project(tools VERSION 0.1.0 LANGUAGES C CXX)


#find_package(Clang REQUIRED CONFIG)

# 自动递归查找所有源码文件
file(GLOB_RECURSE SRC_FILES
        ${PROJECT_SOURCE_DIR}/src/*.cpp
        ${PROJECT_SOURCE_DIR}/src/*.cc
        ${PROJECT_SOURCE_DIR}/src/*.c

        ${CODE_GEN}/*.cpp
        ${CODE_GEN}/*.cc
        ${CODE_GEN}/*.c
)
# 生成可执行文件
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)
# 链接操作
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_LINK_LIBS})