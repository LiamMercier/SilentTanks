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
