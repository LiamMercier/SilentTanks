#include <string>
#include <iostream>

#include "game-instance.h"

int main()
{
    Direction d1 = Direction::North;

    Tank t1(2,1,d1);
    Tank t2(3,4,d1);

    Tank* test_tanks = new Tank[2]{t1, t2};


    GameInstance test(7,5, std::string("envs/testenv.txt"), test_tanks, 2);

    test.print_instance_console();



}
