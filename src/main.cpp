#include "flat-array.h"
#include <iostream>

int main()
{

    FlatArray<int> test(3,4);

    for (int x = 0; x < 3; x++){
        for (int y = 0; y < 4; y++)
        {
            std::cout << test[test.idx(x,y)] << " ";
        }
        std::cout << std::endl;
    }
}
