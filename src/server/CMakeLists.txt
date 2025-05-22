add_executable(SilentTanks-Server main-server.cpp match-instance.cpp session.cpp message.cpp server.cpp match-maker.cpp match-strategy.cpp match-result-recorder.cpp user-manager.cpp database.cpp)

target_include_directories(SilentTanks-Server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(SilentTanks-Server PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(SilentTanks-Server PRIVATE ${Argon2_INCLUDE_DIRS})
target_include_directories(SilentTanks-Server PRIVATE ${libsodium_INCLUDE_DIRS})

target_link_libraries(SilentTanks-Server PRIVATE
    libgame
    Boost::system
    ${ARGON2_LIBRARIES}
    ${libpqxx_LIBRARIES}
    ${libsodium_LIBRARIES}
)
