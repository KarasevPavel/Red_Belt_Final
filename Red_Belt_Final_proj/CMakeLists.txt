cmake_minimum_required(VERSION 3.17)
project(Red_Belt_Final)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O0")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread")

add_executable(Red_Belt_Final main.cpp test_runner.h parse.cpp parse.h search_server.cpp search_server.h iterator_range.h profile.h)