cmake_minimum_required(VERSION 3.16)
project(lesson06_mvp CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(lesson06
    src/page.cpp
    src/disk_manager.cpp
    src/buffer_pool_manager.cpp
    src/lru_replacer.cpp
    src/btree.cpp
    src/catalog.cpp
    src/lexer.cpp
    src/parser.cpp
    src/executor.cpp
    main.cpp
)

target_include_directories(lesson06 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
