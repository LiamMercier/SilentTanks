
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(SilentTanks-Client main-client.cpp client-session.cpp client.cpp client-data.cpp game-manager.cpp gui-client.cpp client-state.h user-list-model.cpp message-model.cpp match-history-model.cpp replay-manager.cpp server-list.cpp ${QML_RCC})

target_include_directories(SilentTanks-Client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(SilentTanks-Client PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(SilentTanks-Client PRIVATE
    libgame
    protocol
    Boost::system
    glaze::glaze
    ${ARGON2_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(SilentTanks-Client PRIVATE DEV_BUILD=1)
endif()
