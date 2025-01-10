#include "game-instance.h"
#include <string>
#include <iostream>

int main()
{

    GameInstance test(5,7, std::string("envs/testenv.txt"));

    test.print_instance_console();



}
