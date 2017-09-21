#include <iostream>

#include "FSM.h"
#include "FSMParser.h"
int main()
{
    FSM test = FSMParser("../misc/linearresult.fs").readFSM();
    test.run();
    return 0;
}