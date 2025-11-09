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

find_path(ARGON2_INCLUDE_DIR
    NAMES argon2.h
    HINTS
        ENV
        ARGON2_ROOT
        ${CMAKE_INSTALL_PREFIX}
        /usr/local
        /usr
)

find_library(ARGON2_LIBRARY
NAMES argon2
HINTS
    ENV
    ARGON2_ROOT
    ${CMAKE_INSTALL_PREFIX}/lib
    /usr/local/lib
    /usr/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Argon2
    REQUIRED_VARS ARGON2_INCLUDE_DIR ARGON2_LIBRARY
)

if(ARGON2_FOUND)
    set(ARGON2_LIBRARIES ${ARGON2_LIBRARY})
    set(ARGON2_INCLUDE_DIRS ${ARGON2_INCLUDE_DIR})
endif()
