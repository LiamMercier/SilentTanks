cmake_minimum_required(VERSION 3.25)

project(SilentTanks CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to release if nothing is set.
if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

if(WIN32)
  set(TARGET_PLATFORM "windows" CACHE STRING "Target platform")
endif()

if(NOT TARGET_PLATFORM)
  set(TARGET_PLATFORM "debian" CACHE STRING "Target platform")
endif()

set_property(CACHE TARGET_PLATFORM PROPERTY STRINGS debian windows linux)

set(valid_platforms debian windows linux)
list(FIND valid_platforms "${TARGET_PLATFORM}" _platform_index)
if(_platform_index EQUAL -1)
  message(FATAL_ERROR "Invalid TARGET_PLATFORM: ${TARGET_PLATFORM}. Options: debian, windows, linux")
endif()

# Tell user what platform is being targetted.
message(STATUS "Target platform: ${TARGET_PLATFORM}")

# find libargon2
set(CMAKE_PREFIX_PATH "/usr/local;/usr")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

file(GLOB TILE_SVGS "${CMAKE_CURRENT_SOURCE_DIR}/qml/svgs/tiles/*.svg")
file(GLOB BUTTON_SVGS "${CMAKE_CURRENT_SOURCE_DIR}/qml/svgs/buttons/*.svg")
file(GLOB PNGS "${CMAKE_CURRENT_SOURCE_DIR}/qml/pngs/*.png")
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

foreach(svg ${BUTTON_SVGS})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/qml" "${svg}")
    file(APPEND "${QML_RCC}" "    <file>${REL_PATH}</file>\n")
endforeach()

foreach(png ${PNGS})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/qml" "${png}")
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

find_package(Argon2 REQUIRED)

find_package(OpenSSL REQUIRED)

find_package(Qt6 REQUIRED COMPONENTS
  Core
  Gui
  Qml
  Quick
  QuickControls2
)

# Add server packages if target is a linux variant.
if(NOT TARGET_PLATFORM STREQUAL "windows")
  find_package(libpqxx REQUIRED)
  find_package(libsodium REQUIRED)

  find_package(Boost REQUIRED COMPONENTS
    system
    program_options
  )

elseif(TARGET_PLATFORM STREQUAL "windows")
  find_package(Boost REQUIRED COMPONENTS
    system
  )

endif()

if(NOT Boost_FOUND)
  message(FATAL_ERROR "Boost not found!")
endif()

if(NOT Argon2_FOUND)
  message(FATAL_ERROR "libargon2 not found!")
endif()

if(NOT OpenSSL_FOUND)
  message(FATAL_ERROR "OpenSSL not found!")
endif()

if(NOT Qt6_FOUND)
  message(FATAL_ERROR "Qt6 not found!")
endif()

# Package finding for server.
if(NOT TARGET_PLATFORM STREQUAL "windows")
  if(NOT libpqxx_FOUND)
    message(FATAL_ERROR "libpqxx not found!")
  endif()

  if(NOT libsodium_FOUND)
    message(FATAL_ERROR "libsodium not found!")
  endif()

endif()

add_subdirectory(src/libgame)
add_subdirectory(src/protocol)

if(NOT TARGET_PLATFORM STREQUAL "windows")
  add_subdirectory(src/server)
endif()

add_subdirectory(src/client)

# packaging
include(InstallRequiredSystemLibraries)

# Generic metadata
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_CONTACT "LiamMercier@proton.me")
set(CPACK_PACKAGE_NAME "silent-tanks")

# Decide on package generation method.
if(TARGET_PLATFORM STREQUAL "debian")
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Liam Mercier <LiamMercier@proton.me>")

  # Enable dependency discovery
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

  # Components
  set(CPACK_COMPONENTS_ALL server client common)
  set(CPACK_DEB_COMPONENT_INSTALL ON)

  # Common data
  set(CPACK_DEBIAN_COMMON_PACKAGE_MAINTAINER "Liam Mercier <LiamMercier@proton.me>")

  # Server package
  set(CPACK_DEBIAN_SERVER_PACKAGE_MAINTAINER "Liam Mercier <LiamMercier@proton.me>")
  set(CPACK_DEBIAN_SERVER_PACKAGE_CONTROL_EXTRA
      "${CMAKE_SOURCE_DIR}/packaging/server/postinst")

  set(CPACK_DEBIAN_SERVER_PACKAGE_DEPENDS "silent-tanks-common (>= ${CPACK_PACKAGE_VERSION})")

  # Client package
  set(CPACK_DEBIAN_CLIENT_PACKAGE_MAINTAINER "Liam Mercier <LiamMercier@proton.me>")

  set(CPACK_DEBIAN_CLIENT_PACKAGE_DEPENDS "silent-tanks-common (>= ${CPACK_PACKAGE_VERSION})")

  # Bring over environment files
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/envs"
          DESTINATION "share/silent-tanks"
          COMPONENT common)

  install(FILES "${CMAKE_SOURCE_DIR}/mapfile.txt"
          DESTINATION "share/silent-tanks"
          COMPONENT common)

  install(FILES "${CMAKE_SOURCE_DIR}/README.md"
          DESTINATION "share/silent-tanks"
          COMPONENT common)

# Handle windows packaging.
elseif(TARGET_PLATFORM STREQUAL "windows")
  set(CPACK_GENERATOR "NSIS")

  set(CPACK_NSIS_DISPLAY_NAME "Silent Tanks")
  set(CPACK_NSIS_PACKAGE_NAME "silent-tanks")
  set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
  set(CPACK_NSIS_MODIFY_PATH ON)

  # Bring over environment files
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/envs"
          DESTINATION "share/silent-tanks")

  install(FILES "${CMAKE_SOURCE_DIR}/mapfile.txt"
          DESTINATION "share/silent-tanks")

  install(FILES "${CMAKE_SOURCE_DIR}/README.md"
          DESTINATION "share/silent-tanks")

  set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
  "SetOutPath \"$LOCALAPPDATA\\silent-tanks\"\n"
  "File /r \"${CMAKE_CURRENT_BINARY_DIR}/share/*\"\n"
  )

endif()

include(CPack)
