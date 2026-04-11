cmake_minimum_required(VERSION 3.16)
project(lesson02_buffer_pool CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(lesson02
    lru_replacer.cpp
    buffer_pool_manager.cpp
    main.cpp
)

target_include_directories(lesson02 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
