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

    vector<Token> st = lexer.tokenize("../misc/test1.f");
    Compiler c(st);
    stringstream pleaseWork;
    c.compile(pleaseWork);
    cout << pleaseWork.str();
    return 0;
}