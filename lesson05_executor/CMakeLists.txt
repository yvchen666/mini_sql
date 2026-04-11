cmake_minimum_required(VERSION 3.16)
project(lesson05_executor CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(lesson05
    catalog.cpp
    executor.cpp
    main.cpp
)

target_include_directories(lesson05 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
