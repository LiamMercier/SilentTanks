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

find_path(libsodium_INCLUDE_DIR
    NAMES sodium.h
    PATH_SUFFIXES include
)

find_library(libsodium_LIBRARY
    NAMES sodium libsodium
    PATH_SUFFIXES lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libsodium
    REQUIRED_VARS libsodium_LIBRARY libsodium_INCLUDE_DIR
)

if(libsodium_FOUND)
    set(libsodium_LIBRARIES ${libsodium_LIBRARY})
    set(libsodium_INCLUDE_DIRS ${libsodium_INCLUDE_DIR})
endif()
