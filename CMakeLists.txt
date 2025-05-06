cmake_minimum_required(VERSION 3.25)

project(SilentTanks CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++11")

set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")

include(FetchContent)

FetchContent_Declare(asio
GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
GIT_TAG asio-1-34-2
GIT_SHALLOW TRUE
)

FetchContent_GetProperties(asio)
if (NOT asio_POPULATED)
  FetchContent_Populate(asio)
endif()

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE
  ${asio_SOURCE_DIR}/asio/include
)

target_compile_definitions(asio INTERFACE
  ASIO_STANDALONE
  ASIO_NO_DEPRECATED
)

find_package(Threads REQUIRED)
# Asio needs threading support
target_link_libraries(asio INTERFACE
  Threads::Threads
)

add_library(asio::asio ALIAS asio)

add_subdirectory(src/libgame)
add_subdirectory(src/server)
add_subdirectory(src/client)
