#include "flat-array.h"
#include "environment.h"
#include <iostream>

int main()
{

    Environment test(3,4);

    for (int x = 0; x < 3; x++){
        for (int y = 0; y < 4; y++)
        {
            std::cout << +((uint8_t) (test[test.idx(x,y)]).type_) << " ";
        }
        std::cout << std::endl;
    }
}
