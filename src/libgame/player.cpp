#include "player.h"

    Player::Player(uint8_t num_tanks, uint8_t ID)
    :tanks_placed_(0), owned_tank_IDs_(new (std::nothrow) int[num_tanks]), num_tanks_(num_tanks), player_ID_(ID)
    {
        // check that memory allocation succeeded
        if (owned_tank_IDs_ == nullptr)
        {
            std::cerr << "player: Memory allocation failed\n";
            return;
        }
        for (int i = 0; i < num_tanks; i++)
        {
            owned_tank_IDs_[i] = NO_TANK;
        }
    }

    Player::Player(const Player& other)
    {
        player_ID_ = other.player_ID_;
        num_tanks_ = other.num_tanks_;
        owned_tank_IDs_ = new int[num_tanks_];
        tanks_placed_ = other.tanks_placed_;

        for (int i = 0; i < num_tanks_; i++)
        {
            owned_tank_IDs_[i] = other.owned_tank_IDs_[i];
        }
    }

    Player::~Player()
    {
        delete[] owned_tank_IDs_;
    }

    int* Player::get_tanks_list()
    {
        return owned_tank_IDs_;
    }

    const int* Player::get_tanks_list() const
    {
        return owned_tank_IDs_;
    }
