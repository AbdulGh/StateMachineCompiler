#include <iostream>

#include "FSM.h"
#include "FSMParser.h"
int main()
{
    FSM test = FSMParser("../misc/stacktest.fs").readFSM();
    test.run();
    return 0;
}