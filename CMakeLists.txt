cmake_minimum_required(VERSION 3.25)

project(SilentTanks CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++11")

# find libargon2
set(CMAKE_PREFIX_PATH "/usr/local;/usr")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")

file(GLOB TILE_SVGS "${CMAKE_CURRENT_SOURCE_DIR}/qml/svgs/tiles/*.svg")
file(GLOB SOUNDS "${CMAKE_CURRENT_SOURCE_DIR}/qml/sounds/*.wav")
file(GLOB QML_FILES "${CMAKE_CURRENT_SOURCE_DIR}/qml/*.qml")

set(QML_RCC ${CMAKE_SOURCE_DIR}/qml/qml-resources.qrc)

set(QML_DIR ${CMAKE_SOURCE_DIR}/qml)

file(WRITE "${QML_RCC}" "<RCC>\n  <qresource prefix=\"\">\n")

foreach(qmlfile ${QML_FILES})
  file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/qml" "${qmlfile}")
  file(APPEND "${QML_RCC}" "    <file>${REL_PATH}</file>\n")
endforeach()

foreach(svg ${TILE_SVGS})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/qml" "${svg}")
    file(APPEND "${QML_RCC}" "    <file>${REL_PATH}</file>\n")
endforeach()

foreach(wav ${SOUNDS})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/qml" "${wav}")
    file(APPEND "${QML_RCC}" "    <file>${REL_PATH}</file>\n")
endforeach()

file(APPEND "${QML_RCC}" "  </qresource>\n</RCC>\n")

include(FetchContent)

FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG main
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glaze)

find_package(Boost REQUIRED COMPONENTS system)

find_package(Argon2 REQUIRED)

find_package(libpqxx REQUIRED)

find_package(libsodium REQUIRED)

find_package(OpenSSL REQUIRED)

find_package(Qt6 REQUIRED COMPONENTS
  Core
  Gui
  Qml
  Quick
  QuickControls2
)

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

if(NOT Qt6_FOUND)
  message(FATAL_ERROR "Qt6 not found!")
endif()

add_subdirectory(src/libgame)
add_subdirectory(src/protocol)
add_subdirectory(src/server)
add_subdirectory(src/client)
