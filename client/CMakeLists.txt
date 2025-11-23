cmake_minimum_required(VERSION 3.16)
project(client VERSION 0.1.0 LANGUAGES C CXX)

set(SOLUTION_DIR ${PROJECT_SOURCE_DIR}/..)

find_package(protobuf CONFIG REQUIRED)

# 源码
file(GLOB_RECURSE SRC_FILES
        ${PROJECT_SOURCE_DIR}/src/*.cpp
        ${PROJECT_SOURCE_DIR}/src/*.cc
        ${PROJECT_SOURCE_DIR}/src/*.c

        ${SOLUTION_DIR}/proto-gen/*.cpp
        ${SOLUTION_DIR}/proto-gen/*.cc
        ${SOLUTION_DIR}/proto-gen/*.c
)

add_executable(${PROJECT_NAME}
        ${SRC_FILES}
)

# 统一头文件路径
target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${SOLUTION_DIR}/proto-gen
)
target_link_directories(${PROJECT_NAME} PRIVATE ${SOLUTION_DIR}/libs)
# 链接操作
set(PROJECT_LINK_LIBS  engine-lua protobuf::libprotobuf)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_LINK_LIBS})
