cmake_minimum_required(VERSION 3.25)

project(SilentTanks CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++11")

# find libargon2
set(CMAKE_PREFIX_PATH "/usr/local;/usr")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")

find_package(Boost REQUIRED COMPONENTS system)

find_package(Argon2 REQUIRED)

find_package(libpqxx REQUIRED)

find_package(libsodium REQUIRED)

find_package(OpenSSL REQUIRED)

if(NOT Boost_FOUND)
  message(FATAL_ERROR "Boost not found!")
endif()

if(NOT Argon2_FOUND)
  message(FATAL_ERROR "libargon2 not found!")
endif()

if(NOT libpqxx_FOUND)
  message(FATAL_ERROR "libpqxx not found!")
endif()

if(NOT libsodium_FOUND)
  message(FATAL_ERROR "libsodium not found!")
endif()

if(NOT OpenSSL_FOUND)
  message(FATAL_ERROR "OpenSSL not found!")
endif()

add_subdirectory(src/libgame)
add_subdirectory(src/server)
add_subdirectory(src/client)
