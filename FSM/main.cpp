#include <iostream>

#include "FSM.h"

using namespace std;

int main(int argc, char** argv)
{
    if (argc != 2) throw runtime_error("Exactly one argument reqiured (filename)");
    string filename = argv[1];
    FSM test(filename);
    test.run();
    return 0;
}