cmake_minimum_required(VERSION 3.25)

project(SilentTanks CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++11")

set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")

find_package(Boost REQUIRED COMPONENTS system)

find_package(argon2 REQUIRED)

if(NOT Boost_FOUND)
  message(FATAL_ERROR "Boost not found!")
endif()

if(NOT argon2_FOUND)
  message(FATAL_ERROR "libargon2 not found!")
endif()

add_subdirectory(src/libgame)
add_subdirectory(src/server)
add_subdirectory(src/client)
