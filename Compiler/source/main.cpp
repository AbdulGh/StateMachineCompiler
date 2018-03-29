#include <iostream>
#include "compile/Compiler.h"
#include "compile/Lexer.h"
#include "symbolic/SymbolicArray.h"
using namespace std;

//todo convert thrown strings to runtime_errors
int main()
{
    Lexer lexer;
    vector<Token> st = lexer.tokenize("/home/abdul/CLionProjects/Compiler/examples/loop examples/twovars.f");
    Compiler c(st, "../warnings.txt");
    stringstream pleaseWork;
    c.compile(pleaseWork);
    cout << pleaseWork.str();
    return 0;
}