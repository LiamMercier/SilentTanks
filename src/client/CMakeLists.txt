
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(SilentTanks-Client main-client.cpp client-session.cpp client.cpp client-data.cpp game-manager.cpp gui-client.cpp client-state.h user-list-model.cpp message-model.cpp match-history-model.cpp replay-manager.cpp server-list.cpp ${QML_RCC})

target_include_directories(SilentTanks-Client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(SilentTanks-Client PRIVATE ${Boost_INCLUDE_DIRS})

if(TARGET_PLATFORM STREQUAL "windows")
  if(EXISTS "${CLIENT_RC}")
    target_sources(SilentTanks-Client PRIVATE "${CLIENT_RC}")
  else()
    message(WARNING "Windows .rc file not found at ${CLIENT_RC}")
  endif()
endif()

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

# Install locations based on platform.
if(TARGET_PLATFORM STREQUAL "debian")
  install(TARGETS SilentTanks-Client
          RUNTIME DESTINATION bin
          COMPONENT client)

  install(FILES "${CMAKE_SOURCE_DIR}/server-list.txt"
          DESTINATION "share/silent-tanks"
          COMPONENT client)

  # Add desktop file and icon.
  install(FILES "${CMAKE_SOURCE_DIR}/packaging/linux/silent-tanks.desktop"
          DESTINATION "/usr/share/applications"
          COMPONENT client)

  install(FILES "${CMAKE_SOURCE_DIR}/qml/svgs/icons/silent-tanks.svg"
          DESTINATION "/usr/share/icons/hicolor/scalable/apps"
          COMPONENT client)

elseif(TARGET_PLATFORM STREQUAL "windows")
  install(TARGETS SilentTanks-Client
          RUNTIME DESTINATION .)

  install(FILES "${CMAKE_SOURCE_DIR}/server-list.txt"
          DESTINATION "share")
endif()
