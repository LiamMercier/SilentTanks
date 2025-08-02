add_library(libgame STATIC flat-array.cpp grid-cell.cpp game-instance.cpp tank-entity.cpp player.cpp maps.cpp)

target_include_directories(libgame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
