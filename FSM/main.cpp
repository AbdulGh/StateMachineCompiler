#include <iostream>

#include "FSM.h"

using namespace std;

int main()
{
    string filename = "../misc/ackermann.fs";
    FSM test(filename);
    test.run();
    return 0;
}