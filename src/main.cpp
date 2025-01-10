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

    Tank t1(2,1,d1,0);
    Tank t2(3,4,d1,1);
    Tank t3(3,2,d1,2);
    Tank t4(1,0,d1,3);
    Tank t5(0,0,d1,1);

    Tank* test_tanks = new Tank[5]{t1, t2, t3, t4, t5};


    GameInstance test(7,5, std::string("envs/testenv.txt"), test_tanks, 5);

    test.print_instance_console();

    test.move_tank(1);

    std::cout << "\n";

    test.print_instance_console();

}
