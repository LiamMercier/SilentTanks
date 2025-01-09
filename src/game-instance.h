#pragma once

#include "environment.h"

// The game instance class contains all
// relevant state data for a given game.
class GameInstance
{

    GameInstance(Environment preload);

    ~GameInstance();

private:
    Environment game_env_;
};
