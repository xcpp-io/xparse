#include "test1_a.h"

#include <iostream>

namespace Test {

const Vector3 Vector3::Zero = Vector3 { 0.0, 0.0, 0.0 };
const Vector3 Vector3::One  = Vector3 { 1.0, 1.0, 1.0 };

void GlobalTestFunction()
{
    std::cout << "Hello Test1!\n";
}

} // namespace Test