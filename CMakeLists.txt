cmake_minimum_required(VERSION 3.16)
project(sql_mvp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
add_compile_options(-Wall -Wextra -O2)

# 添加各课子目录
add_subdirectory(lesson01_storage)
add_subdirectory(lesson02_buffer_pool)
add_subdirectory(lesson03_btree)
add_subdirectory(lesson04_sql_parser)
add_subdirectory(lesson05_executor)
add_subdirectory(lesson06_mvp)
