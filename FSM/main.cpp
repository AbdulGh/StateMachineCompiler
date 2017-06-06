#include <iostream>

#include "FSM.h"
#include "FSMParser.h"
int main()
{
    FSM test = FSMParser("../misc/fizz2.fs").readFSM();
    test.run();
    return 0;
}