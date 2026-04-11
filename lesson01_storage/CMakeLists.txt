cmake_minimum_required(VERSION 3.16)
project(lesson01_storage CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(lesson01
    page.cpp
    disk_manager.cpp
    main.cpp
)

target_include_directories(lesson01 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
