#pragma once

#include "maps.h"

class MapRepository
{
public:
    MapRepository() = default;

    void load_map_file(std::string map_file_name);

    const std::vector<GameMap> & get_available_maps() const;

    // in the future, perhaps allow for adding maps to the repository

private:
    std::vector<GameMap> maps_;
};
