add_library(protocol STATIC header.cpp message.cpp server-identity.cpp)

target_include_directories(protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(protocol PRIVATE
    libgame
    glaze::glaze
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(protocol PRIVATE DEV_BUILD=1)
endif()
