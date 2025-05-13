add_executable(SilentTanks-Server main-server.cpp match-instance.cpp session.cpp message.cpp server.cpp match-maker.cpp match-strategy.cpp)
target_include_directories(SilentTanks-Server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SilentTanks-Server PRIVATE libgame asio::asio)
