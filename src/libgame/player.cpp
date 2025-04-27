#include "player.h"

    Player::Player()
    {
    }

    Player::Player(uint8_t num_tanks, uint8_t ID)
    :owned_tank_IDs_(new (std::nothrow) int[num_tanks]), num_tanks_(num_tanks), player_ID_(ID)
    {
        // check that memory allocation succeeded
        if (owned_tank_IDs_ == nullptr)
        {
            std::cerr << "player: Memory allocation failed\n";
            return;
        }
        // now, let something else populate tanks_ with useful values
    }

    Player::Player(const Player& other)
    {
        player_ID_ = other.player_ID_;
        num_tanks_ = other.num_tanks_;

        owned_tank_IDs_ = new int[num_tanks_];

        for (int i = 0; i < num_tanks_; i++)
        {
            owned_tank_IDs_[i] = other.owned_tank_IDs_[i];
        }
    }

    Player::~Player()
    {
        // environment handles clean up, not players.
        owned_tank_IDs_ = nullptr;
    }

    int* Player::get_tanks_list()
    {
        return owned_tank_IDs_;
    }
