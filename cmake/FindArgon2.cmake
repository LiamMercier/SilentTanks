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
