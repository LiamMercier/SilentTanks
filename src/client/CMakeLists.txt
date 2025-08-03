 
add_executable(SilentTanks-Client main-client.cpp client-session.cpp client.cpp client-data.cpp game-manager.cpp)

target_include_directories(SilentTanks-Client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(SilentTanks-Client PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(SilentTanks-Client PRIVATE
    libgame
    protocol
    Boost::system
    glaze::glaze
    ${ARGON2_LIBRARIES}
)
