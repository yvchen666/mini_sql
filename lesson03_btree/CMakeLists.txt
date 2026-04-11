cmake_minimum_required(VERSION 3.16)
project(lesson03_btree CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(lesson03
    btree.cpp
    main.cpp
)

target_include_directories(lesson03 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
