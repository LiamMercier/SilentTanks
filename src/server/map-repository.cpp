#include "map-repository.h"
#include "message.h"
#include "asset-resolver.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <stdint.h>

MapRepository::MapRepository()
:maps_(NUMBER_OF_MODES)
{

}

// TODO <refactoring>: make a map file format that isn't reprehensible.
void MapRepository::load_map_file(std::string map_file_name)
{
    // Lock all maps.
    std::unique_lock lock{maps_mutex_};

    std::error_code ec;

    auto mapfile_path = AppAssets::resolve_asset(map_file_name);

    if (mapfile_path.empty())
    {
        std::cerr << "Map file does not exist\n";
        throw std::runtime_error("Map file does not exist.");
    }

    auto size = std::filesystem::file_size(mapfile_path, ec);

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

    std::ifstream in_file(mapfile_path);

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
    while (in_file >> name >> w >> h >> tanks >> players >> mode)
    {
        std::string map_path_name = "envs/" + name;
        std::filesystem::path map_path = AppAssets::resolve_asset(map_path_name);

        // Check that numbers are valid for their datatype.
        if (w <= 0 || w >= UINT8_MAX ||
            h <= 0 || h >= UINT8_MAX ||
            tanks <= 0 || tanks >= UINT8_MAX ||
            players <= 0 || players >= UINT8_MAX ||
            mode < 0 || mode >= NUMBER_OF_MODES)
        {
            std::cerr << "Environment file " << map_path_name
                        << " has invalid values\n";
            throw std::runtime_error("Environment file has invalid values.");
        }

        uint16_t total = w * h;

        // Check that the number of players is actually valid
        // for the supposed game mode.
        if (players_for_gamemode[mode] != players)
        {
            std::cerr << "Environment file " << map_path
                        << " has an invalid number of players\n";
            throw std::runtime_error("Environment file has invalid players.");
        }

        // See if there will be more tanks than tank IDs.
        if (size_t(players) * size_t(tanks) >= size_t(UINT8_MAX))
        {
            std::cerr << "Environment file " << map_path
                        << " has too many tanks\n";
            throw std::runtime_error("Environment file has too many tanks.");
        }

        std::error_code ec;

        if (!std::filesystem::is_regular_file(map_path, ec))
        {
            std::cerr << "Environment file " << map_path
                        << " not found or not regular\n";
            throw std::runtime_error("Environment file not found.");
        }

        auto size = std::filesystem::file_size(map_path, ec);

        if (ec)
        {
            std::cerr << "Unable to query file size for "
                        << map_path << "\n";
            throw std::runtime_error("Unable to query environment file size.");
        }

        if (size != static_cast<std::size_t>(total) * 2)
        {
            std::cerr << "File size mismatch for "
                        << map_path << "\n";
            throw std::runtime_error("File size mismatch in environment file.");
        }

        std::ifstream file(map_path, std::ios::binary);

        if (!file.is_open())
        {
            std::cerr << "Error opening " << map_path << "\n";
            throw std::runtime_error("Error opening environment file.");
        }

        std::vector<char> env_buffer(total);
        file.read(env_buffer.data(), total);

        if (file.gcount() != total)
        {
            std::cerr << "Too few bytes reading environment from "
                        << map_path << "\n";
            throw std::runtime_error("Too few bytes in environment file.");
        }

        if (!file)
        {
            std::cerr << "IO error reading environment from "
                        << map_path << "\n";
            throw std::runtime_error("IO error reading environment from.");
        }

        // Read bitmask for tank placements.
        std::vector<char> mask_buffer(total);
        file.read(mask_buffer.data(), total);

        if (file.gcount() != total)
        {
            std::cerr << "Too few bytes reading mask from "
                        << map_path << "\n";
            throw std::runtime_error("Too few bytes in environment file.");
        }

        if (!file)
        {
            std::cerr << "IO error reading mask from "
                        << map_path << "\n";
            throw std::runtime_error("IO error reading environment from.");
        }

        MapSettings settings(name,
                             static_cast<uint8_t>(w),
                             static_cast<uint8_t>(h),
                             static_cast<uint8_t>(tanks),
                             static_cast<uint8_t>(players),
                             static_cast<uint8_t>(mode));

        GameMap this_map(settings);

        // turn the ASCII data into the correct integers
        // and assign the GridCell elements the correct type accordingly.
        for (uint16_t i = 0; i < total; i++)
        {
            char c = env_buffer[i];

            if (c < '0' || c > '2')
            {
                std::cerr << "Invalid environment character in "
                            << map_path << "\n";
                throw std::runtime_error("Invalid environment character.");
            }

            uint8_t value = static_cast<uint8_t>(c - '0');

            this_map.env[i].type_ = static_cast<CellType>(value);
            this_map.env[i].occupant_ = NO_OCCUPANT;
            this_map.env[i].visible_ = true;

            // Convert the bitmask for each player.
            //
            // Places where a player may place their tank are identified
            // by their player ID.
            this_map.mask[i] = static_cast<uint8_t>(mask_buffer[i] - '0');
        }

        // Add the new map to the list of maps.
        maps_[static_cast<uint8_t>(mode)].push_back(std::move(this_map));

    }

    if (in_file.bad())
    {
        std::cerr << "IO error while reading mapfile.\n";
        throw std::runtime_error("IO error while reading mapfile.");
    }

    for (size_t i = 0; i < maps_.size(); i++)
    {
        if (maps_[i].size() < 1)
        {
            std::cerr << "No maps found for queue " << +i << ".\n";
            throw std::runtime_error("No maps found.");
        }
    }

}

// Initialize rng generation source.
thread_local std::mt19937 MapRepository::gen_{std::random_device{}()};

const GameMap & MapRepository::get_random_map(uint8_t index) const
{
    std::shared_lock lock{maps_mutex_};

    // Sample uniformly from the number of maps available.
    const auto & bucket = maps_[index];
    std::uniform_int_distribution<size_t> dist(0, bucket.size() - 1);
    size_t i = dist(gen_);

    return bucket[i];
}
