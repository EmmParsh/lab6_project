cmake_minimum_required(VERSION 3.10)

project(ConsoleFileManager)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(ConsoleFileManager
    src/main.cpp
    src/FileManager.cpp
)
