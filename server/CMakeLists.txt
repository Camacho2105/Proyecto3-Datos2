cmake_minimum_required(VERSION 3.10)

project(TinySQLDb)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(
    TinySQLDb
    src/main.cpp
    src/parser.cpp
    src/catalog.cpp
    src/storage.cpp
    src/list.cpp
)