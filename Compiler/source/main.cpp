#include <iostream>

#include "compile/Compiler.h"

using namespace std;

int main()
{
    Lexer lexer;
    vector<Token> st = lexer.tokenize("../examples/loop examples/loop2.f");
    Compiler c(st, "../warnings.txt");
    stringstream pleaseWork;
    c.compile(pleaseWork);
    cout << pleaseWork.str();
    return 0;
}