#include <string>
#include <iostream>

#include "game-instance.h"

int main()
{
    uint8_t d1 = 1;

    Player* players = new Player[2];
    Tank* total_tanks = new Tank[2*2];

    uint8_t tank_start_x[2*2]{4,11,6,11};
    uint8_t tank_start_y[2*2]{2,5,7,6};


    for (uint8_t player_num = 0; player_num < 2; player_num++)
    {
        Player this_player(2,player_num);
        players[player_num] = this_player;

        int* this_tanks = this_player.get_tanks_list();

        for (uint8_t tank = 0; tank < 2; tank++)
        {
            std::cout << "TANK " << +tank << "\n";
            std::cout << "PLAYER " << +player_num << "\n";
            std::cout << "TOTAL TANKS INDX" << 2*player_num + tank << "\n";
            total_tanks[2*player_num + tank] = Tank(tank_start_x[2*player_num + tank], tank_start_y[2*player_num + tank], d1, player_num);
            this_tanks[tank] = 2*player_num + tank;

        }
    }

    // Now, we test that:

    // - Each player has the correct tanks
    // - The tanks are in the correct positions
    // - The board has them occupying the correct spots
    // - The tank list is good
    // - Each tank has the right ID

    GameInstance test(12,8, std::string("envs/test2.txt"), total_tanks, 4, players, 2);



    test.print_instance_console();

    bool could_move = test.move_tank(3);

    std::cout << "Tank 3 moving up right: " << could_move;

    std::cout << "\n";

    test.print_instance_console();

    test.rotate_tank(2, 0);

    could_move = test.move_tank(2);

    std::cout << "Tank 2 moving right: " << could_move;

    std::cout << "\n";

    test.print_instance_console();

    test.rotate_tank(2, 0);

    could_move = test.move_tank(2);

    std::cout << "Tank 2 moving down right: " << could_move;

    std::cout << "\n";

    test.print_instance_console();

    test.rotate_tank(2, 0);

    could_move = test.move_tank(2);

    std::cout << "Tank 2 moving down: " << could_move;

    std::cout << "\n";

    test.print_instance_console();

    test.rotate_tank(2, 0);

    could_move = test.move_tank(2);

    std::cout << "Tank 2 moving down left: " << could_move;

    std::cout << "\n";

    test.print_instance_console();

    test.rotate_tank(2, 0);

    could_move = test.move_tank(2);

    std::cout << "Tank 2 moving left: " << could_move;

    std::cout << "\n";

    test.print_instance_console();

    test.rotate_tank(2, 0);

    could_move = test.move_tank(2);

    std::cout << "Tank 2 moving up left: " << could_move;

    std::cout << "\n";

    test.print_instance_console();

    test.rotate_tank(2, 0);

    could_move = test.move_tank(2);

    std::cout << "Tank 2 moving up: " << could_move;

    std::cout << "\n";

    test.print_instance_console();

    test.rotate_tank(2, 0);

    could_move = test.move_tank(2);

    std::cout << "Tank 2 moving up right: " << could_move;

    std::cout << "\n";

    test.print_instance_console();

    test.rotate_tank(2, 1);

    could_move = test.move_tank(2);

    std::cout << "Tank 2 moving up: " << could_move;

    std::cout << "\n";

    test.print_instance_console();

    test.rotate_tank(2, 1);

    test.rotate_tank(2, 1);

    could_move = test.move_tank(2);

    could_move = test.move_tank(2);

    std::cout << "Tank 2 moving left twice: " << could_move;

    std::cout << "\n";

    test.print_instance_console();

    test.rotate_tank(2, 1);

    could_move = test.move_tank(2);

    std::cout << "Tank 2 moving down left: " << could_move;

    std::cout << "\n";

    test.print_instance_console();

    could_move = test.move_tank(0);

    test.rotate_tank(0, 1);

    could_move = test.move_tank(0);

    test.rotate_tank(0, 1);

    could_move = test.move_tank(0);

    test.rotate_tank(0, 1);

    could_move = test.move_tank(0);

    test.rotate_tank(0, 1);

    could_move = test.move_tank(0);

    test.rotate_tank(0, 1);

    could_move = test.move_tank(0);

    test.rotate_tank(0, 1);

    could_move = test.move_tank(0);

    test.rotate_tank(0, 1);

    could_move = test.move_tank(0);

    std::cout << "Nothing should move";

    std::cout << "\n";

    test.rotate_tank_barrel(0, 0);
    test.rotate_tank_barrel(0, 0);


    test.print_instance_console();

    // fire tank 0

    bool fire_hit = test.fire_tank(0);

    fire_hit = test.fire_tank(0);

    std::cout << "FIRING TANK 0 twice \n";

    test.print_instance_console();

    fire_hit = test.fire_tank(0);

    std::cout << "FIRING TANK 0 \n";

    test.print_instance_console();

    fire_hit = test.fire_tank(0);

    std::cout << "FIRING TANK 0 \n";

    test.rotate_tank_barrel(0, 1);

    std::cout << "Rotating tank 0 barrel \n";

    test.rotate_tank_barrel(0, 1);

    test.rotate_tank_barrel(0, 1);

    //test.rotate_tank_barrel(0, 1);

    //test.rotate_tank_barrel(0, 1);

    test.rotate_tank_barrel(1, 0);

    test.rotate_tank_barrel(1, 0);

    test.rotate_tank_barrel(1, 0);

    test.print_instance_console();

    std::cout<<"\n";

    Environment test_view = test.compute_view(0);

    /*

    std::cout << "printing cell types \n\n";

    for (int a = 0; a < 8; a++)
    {
        for (int b = 0; b < 12; b++){
            GridCell curr = test_view[b + (a*12)];
            std::cout << curr << " ";
        }
        std::cout << "\n";

    }

    std::cout << "\noccupants \n";

    for (int a = 0; a < 8; a++)
    {
        for (int b = 0; b < 12; b++){
            GridCell curr = test_view[b + (a*12)];
            if (curr.occupant_ == UINT8_MAX)
            {
                std::cout << "_" << " ";
            }
            else
            {
                std::cout << +curr.occupant_ << " ";
            }

        }
        std::cout << "\n";

    }

    std::cout << "\nvisibility \n";

    for (int a = 0; a < 8; a++)
    {
        for (int b = 0; b < 12; b++){
            GridCell curr = test_view[b + (a*12)];
            if (curr.visible_ == true)
            {
                std::cout << "_" << " ";
            }
            else
            {
                std::cout << "?" << " ";
            }

        }
        std::cout << "\n";

    }

    */


    std::cout << "Computing view for player 0 \n\n";
    test_view.print_view(total_tanks);

    test.rotate_tank_barrel(1, 0);

    test.rotate_tank_barrel(1, 0);

    test.rotate_tank_barrel(0, 0);

    test.rotate_tank_barrel(0, 0);

    test_view = test.compute_view(0);

    std::cout << "Computing view for player 0 \n\n";
    test_view.print_view(total_tanks);

    test.rotate_tank_barrel(1, 0);

    test.rotate_tank_barrel(1, 0);

    test.rotate_tank_barrel(0, 0);

    test.rotate_tank_barrel(0, 0);

    test_view = test.compute_view(0);

    std::cout << "Computing view for player 0 \n\n";
    test_view.print_view(total_tanks);

    test.rotate_tank_barrel(1, 0);

    test.rotate_tank_barrel(1, 0);

    test.rotate_tank_barrel(0, 0);

    test.rotate_tank_barrel(0, 0);

    test_view = test.compute_view(0);

    std::cout << "Computing view for player 0 \n\n";
    test_view.print_view(total_tanks);

    test.rotate_tank_barrel(1, 1);

    test.rotate_tank(1,1);

    test.rotate_tank(1,1);

    test.rotate_tank(1,1);

    test.move_tank(1);

    test.move_tank(1);

    test.move_tank(1);

    test.move_tank(1);

    test.move_tank(1);

    test.print_instance_console();

    test_view = test.compute_view(0);

    std::cout << "Computing view for player 0 \n\n";
    test_view.print_view(total_tanks);

    std::cout << "Rotating tank 1\n\n";

    test.rotate_tank_barrel(1,1);

    test.rotate_tank_barrel(1,1);

    test_view = test.compute_view(0);

    std::cout << "Computing view for player 0 \n\n";
    test_view.print_view(total_tanks);

        std::cout << "Rotating tank 1\n\n";

    test.rotate_tank_barrel(1,1);

    test.rotate_tank_barrel(1,1);

    test_view = test.compute_view(0);

    std::cout << "Computing view for player 0 \n\n";
    test_view.print_view(total_tanks);

        std::cout << "Rotating tank 1\n\n";

    test.rotate_tank_barrel(1,1);

    test.rotate_tank_barrel(1,1);

    test_view = test.compute_view(0);

    std::cout << "Computing view for player 0 \n\n";
    test_view.print_view(total_tanks);


}
