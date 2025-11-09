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

add_library(protocol STATIC header.cpp message.cpp server-identity.cpp)

target_include_directories(protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(protocol PRIVATE
    libgame
    glaze::glaze
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(protocol PRIVATE DEV_BUILD=1)
endif()
