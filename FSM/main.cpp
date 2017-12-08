#include <iostream>

#include "FSM.h"

using namespace std;

int main()
{
    string filename = "../misc/mutualEvenOddResult.fs";
    FSM test(filename);
    test.run();
    return 0;
}