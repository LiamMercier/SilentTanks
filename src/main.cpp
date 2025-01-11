#include <string>
#include <iostream>

#include "game-instance.h"

int main()
{
    uint8_t d1 = 0;

    // idea:
    // for player in players
    // create tank(s)
    // append to tanks to start game
    // send through the player list to the GameInstance

    Player* players = new Player[2];
    Tank* total_tanks = new Tank[2*2];


    for (uint8_t player_num = 0; player_num < 2; player_num++)
    {
        Player this_player(2,player_num);
        Tank** this_tanks = this_player.get_tanks_list();

        for (uint8_t tank = 0; tank < 2; tank++)
        {
            std::cout << "TANK " << +tank << "\n";
            std::cout << "PN " << +player_num << "\n";
            std::cout << 2*player_num + tank << "\n";
            total_tanks[2*player_num + tank] = Tank(player_num, tank, d1, player_num);
            this_tanks[tank] = &total_tanks[2*player_num + tank];

        }
    }

    // Now, we test that:

    // - Each player has the correct tanks
    // - The tanks are in the correct positions
    // - The board has them occupying the correct spots
    // - The tank list is good
    // - Each tank has the right ID

    GameInstance test(7,5, std::string("envs/testenv.txt"), total_tanks, 4, players, 2);



    test.print_instance_console();

    test.move_tank(1);

    std::cout << "\n";

    test.print_instance_console();

}
