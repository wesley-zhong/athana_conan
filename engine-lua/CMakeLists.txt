cmake_minimum_required(VERSION 3.16)
project(engine-lua VERSION 0.1.0 LANGUAGES C CXX)


# ========== 路径配置 ==========
set(SOLUTION_DIR ${PROJECT_SOURCE_DIR}/..)
set(SOL2 ${SOLUTION_DIR}/3rd/sol2)
set(LUA ${SOLUTION_DIR}/3rd/lua)
# ========== 源码收集 ==========
# 获取路径下所有的.cpp/.c/.cc文件，并赋值给变量中
aux_source_directory(${PROJECT_SOURCE_DIR} CPP_FILES)


# ========== 生成静态库 ==========
add_library(${PROJECT_NAME} STATIC
        ${CPP_FILES}
)
# ========== include 路径

# ========== 依赖链接 ==========
set(PROJECT_LINK_LIBS  sol2::sol2   dal transport)


# 链接操作
target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_LINK_LIBS})