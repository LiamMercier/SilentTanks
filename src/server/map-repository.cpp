#include "map-repository.h"

#include <iostream>
#include <filesystem>
#include <fstream>

void MapRepository::load_map_file(std::string map_file_name)
{
    std::error_code ec;

    if (!std::filesystem::is_regular_file(map_file_name, ec))
    {
        std::cerr << "Map file not found, closing server.\n";
        throw std::runtime_error("Failed to find map file.");
    }

    auto size = std::filesystem::file_size(map_file_name, ec);

    if (ec)
    {
        std::cerr << "Unable to query map file size.\n";
        throw std::runtime_error("Unable to query map file size.");
    }

    if (size == 0)
    {
        std::cerr << "Map file is empty.\n";
        throw std::runtime_error("Map file is empty.");
    }

    std::ifstream in_file(map_file_name);

    if (!in_file.is_open())
    {
        std::cerr << "Unable to open map file.\n";
        throw std::runtime_error("Unable to open map file.");
    }

    // Close server early on any major issues.
    in_file.exceptions(std::ifstream::badbit);

    std::string name;
    int w;
    int h;
    int tanks;
    int players;
    int mode;

    // Go line by line and read map data.
    while(in_file >> name >> w >> h >> tanks >> players >> mode)
    {
        std::filesystem::path map_path = std::filesystem::path("envs") / name;

        // TODO: check file exists, valid values, etc.

        maps_.emplace_back(map_path.string(),
                           static_cast<uint8_t>(w),
                           static_cast<uint8_t>(h),
                           static_cast<uint8_t>(tanks),
                           static_cast<uint8_t>(players),
                           static_cast<uint8_t>(mode));
    }

    if (in_file.bad())
    {
        std::cerr << "IO error while reading mapfile.\n";
        throw std::runtime_error("IO error while reading mapfile.");
    }

    if (maps_.size() < 1)
    {
        std::cerr << "No maps found.\n";
        throw std::runtime_error("No maps found.");
    }

}

const std::vector<GameMap> & MapRepository::get_available_maps() const
{
    return maps_;
}
