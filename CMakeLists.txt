cmake_minimum_required(VERSION 3.25)

project(SilentTanks CXX)

add_subdirectory(src/libgame)
add_subdirectory(src/server)
add_subdirectory(src/client)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++11")
