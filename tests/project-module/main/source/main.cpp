#include <iostream>

#include "test1_a.h"

int main()
{
    std::cout << "Hello Main!\n";

    Test::Vector3 test_vec;
    test_vec.x = 1.0;

    Test::GlobalTestFunction();

    return 0;
}