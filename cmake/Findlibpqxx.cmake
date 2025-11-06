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

find_path(libpqxx_INCLUDE_DIR
    NAMES pqxx/version.hxx
    PATH_SUFFIXES include
)

find_library(libpqxx_LIBRARY
    NAMES pqxx libpqxx
    PATH_SUFFIXES lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libpqxx
    REQUIRED_VARS libpqxx_LIBRARY libpqxx_INCLUDE_DIR
    VERSION_VAR libpqxx_VERSION
)

if(libpqxx_FOUND)
    set(libpqxx_LIBRARIES ${libpqxx_LIBRARY})
    set(libpqxx_INCLUDE_DIRS ${libpqxx_INCLUDE_DIR})
endif()
