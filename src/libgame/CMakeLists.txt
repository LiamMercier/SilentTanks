add_library(libgame STATIC flat-array.cpp grid-cell.cpp game-instance.cpp tank-entity.cpp player.cpp maps.cpp)

target_include_directories(libgame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(libgame PRIVATE DEV_BUILD=1)
endif()
