#include <iostream>

#include "FSM.h"
#include "FSMParser.h"
int main()
{
    FSM test = FSMParser("../misc/mutualEvenOddResult.fs").readFSM();
    test.run();
    return 0;
}