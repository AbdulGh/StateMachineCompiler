#include <iostream>

#include "Parser.h"

using namespace std;

int main()
{
    Lexer lexer("../misc/test.f");
    Parser p(lexer);
    p.compile();
    return 0;
}