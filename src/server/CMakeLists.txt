# Copyright (c) 2025 Liam Mercier
#
# This file is part of SilentTanks.
#
# SilentTanks is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License Version 3.0
# as published by the Free Software Foundation.
#
# SilentTanks is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License v3.0
# for more details.
#
# You should have received a copy of the GNU Affero General Public License v3.0
# along with SilentTanks. If not, see <https://www.gnu.org/licenses/agpl-3.0.txt>

add_executable(SilentTanks-Server main-server.cpp match-instance.cpp session.cpp server.cpp match-maker.cpp match-strategy.cpp user-manager.cpp database.cpp map-repository.cpp console.cpp elo-updates.cpp)

target_include_directories(SilentTanks-Server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(SilentTanks-Server PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(SilentTanks-Server PRIVATE ${Argon2_INCLUDE_DIRS})
target_include_directories(SilentTanks-Server PRIVATE ${libsodium_INCLUDE_DIRS})

target_link_libraries(SilentTanks-Server PRIVATE
    libgame
    protocol
    Boost::system
    Boost::program_options
    ${ARGON2_LIBRARIES}
    ${libpqxx_LIBRARIES}
    ${libsodium_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    glaze::glaze
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(SilentTanks-Server PRIVATE DEV_BUILD=1)
endif()

if(TARGET_PLATFORM STREQUAL "debian")

  # Bring over the application binary
  install(TARGETS SilentTanks-Server
          RUNTIME DESTINATION lib/silent-tanks
          COMPONENT server
          PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                      GROUP_READ GROUP_EXECUTE)

  # Bring over the script to make certs
  install(PROGRAMS "${CMAKE_SOURCE_DIR}/setup/create-self-signed-cert.sh"
          DESTINATION "share/silent-tanks/setup"
          COMPONENT server
          PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                      GROUP_READ GROUP_EXECUTE)

  # Bring over postgres setup script.
  install(FILES "${CMAKE_SOURCE_DIR}/setup/create-tables.sql"
          DESTINATION "share/silent-tanks/setup"
          COMPONENT server
          PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                      GROUP_READ GROUP_EXECUTE
                      WORLD_READ)

  # Bring over the SQL table setup
  install(FILES "${CMAKE_SOURCE_DIR}/setup/setup-database.sh"
          DESTINATION "share/silent-tanks/setup"
          COMPONENT server
          PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                      GROUP_READ GROUP_EXECUTE)

  # Create user shell wrapper, this is what the end user calls.
  install(PROGRAMS "${CMAKE_SOURCE_DIR}/packaging/server/silent-tanks-server"
          DESTINATION bin
          COMPONENT server)
endif()

# No default server packaging outside of debian.
