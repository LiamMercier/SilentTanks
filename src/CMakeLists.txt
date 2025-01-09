add_executable(SilentTanks main.cpp flat-array.h flat-array.cpp environment.h environment.cpp grid-cell.h grid-cell.cpp cell-type.h game-instance.h game-instance.cpp)

target_link_libraries(SilentTanks stdc++)
