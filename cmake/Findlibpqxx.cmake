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
