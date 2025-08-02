add_library(protocol STATIC header.cpp message.cpp)

target_include_directories(protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(protocol PRIVATE
    libgame
    glaze::glaze
)
