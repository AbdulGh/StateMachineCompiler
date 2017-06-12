#include <iostream>

#include "Parser.h"

using namespace std;

int main()
{
    Lexer lexer("../misc/test.f");
    /*Token n;

    while (n.type != END)
    {
        n = lexer.getNextToken();
        cout << n << endl;
    }*/

    Parser p(lexer);
    p.compile();
    return 0;
}