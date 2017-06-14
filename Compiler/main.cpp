#include <iostream>

#include "Compiler.h"

using namespace std;

int main()
{
    Lexer lexer;

    /*Token n;

    while (n.type != END)
    {
        n = lexer.getNextToken();
        cout << n << endl;
    }*/

    vector<Token> st = lexer.tokenize("../misc/test.f");
    Compiler c(st);
    c.compile();
    return 0;
}