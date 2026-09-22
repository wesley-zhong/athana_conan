cmake_minimum_required(VERSION 3.16)

project(athena VERSION 0.1.0 LANGUAGES C CXX)

# 显式声明 CMP0111(NEW)：导入目标缺 IMPORTED_LOCATION 时在生成期报错，
# 而不是静默生成 <TARGET>-NOTFOUND 到链接命令里。放在 add_subdirectory 之前，
# 以 DEFAULT 变量形式传播——子模块各自的 cmake_minimum_required 重置策略时也生效
set(CMAKE_POLICY_DEFAULT_CMP0111 NEW)

# Conan 2.x 自动生成的依赖文件
# 注意：官方只推荐 include conandeps.cmake
#include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)


# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(3RD ${PROJECT_SOURCE_DIR}/3rd)
set(CODE_GEN ${PROJECT_SOURCE_DIR}/code-gen)


# Dependencies from Conan
# find_package 在此统一执行一次；子模块通过 target_link_libraries 引用
# 导入目标（传递依赖），不再各自 find_package
find_package(spdlog REQUIRED)
find_package(mongocxx REQUIRED)
find_package(libmysqlclient REQUIRED)
find_package(protobuf REQUIRED)
find_package(sol2 REQUIRED)
find_package(hiredis REQUIRED)
find_package(libuv REQUIRED)
find_package(tomlplusplus REQUIRED)
find_package(etcd-cpp-apiv3 REQUIRED)
find_package(RapidJSON REQUIRED)

# 如果因为平台差异没有 uv_a，但有标准的 libuv，则手动做个映射
if(NOT TARGET libuv::uv_a AND TARGET libuv::libuv)
	message(STATUS "Mapping libuv::libuv to libuv::uv_a for compatibility")
	add_library(libuv::uv_a INTERFACE IMPORTED)
	set_target_properties(libuv::uv_a PROPERTIES INTERFACE_LINK_LIBRARIES libuv::libuv)
endif()

# Platform compile settings
if(APPLE)
	add_compile_options(-Wall -Wextra -Wno-unused-parameter -g)
	set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/macos)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/libs/macos)
endif()

if(UNIX AND NOT APPLE)
	add_compile_options(-Wall -Wextra -Wno-unused-parameter -g)
	set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/linux)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/libs/linux)
endif()

if(MSVC)
	# /W4 + 关闭 C4100(未引用形参)，与 Linux 的 -Wno-unused-parameter 对齐；conan_toolchain 已注入 /MP
	add_compile_options(/std:c++20 /utf-8 /Zc:__cplusplus /EHsc /W4 /wd4100)
	add_compile_definitions(NOMINMAX)
	set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/win)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/libs/win)
endif()

# Project submodules
add_subdirectory(core)
add_subdirectory(athena-discovery)
add_subdirectory(dal)
add_subdirectory(transport)
add_subdirectory(engine-lua)
add_subdirectory(game)
add_subdirectory(gateway)
add_subdirectory(client)