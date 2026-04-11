cmake_minimum_required(VERSION 3.16)
project(lesson04_sql_parser CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(lesson04
    lexer.cpp
    parser.cpp
    main.cpp
)

target_include_directories(lesson04 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
