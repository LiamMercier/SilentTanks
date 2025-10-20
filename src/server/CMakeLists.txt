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
