#include <iostream>
#include "compile/Compiler.h"
#include "compile/Lexer.h"

#include "symbolic/SymbolicVariables.h"

using namespace std;

//todo convert thrown strings to runtime_errors
int main()
{
    //Reporter r("../warnings.txt");
    //auto balls = make_unique<SymbolicDouble>("???", r);


    //return 0;

    Lexer lexer;
    vector<Token> st = lexer.tokenize("/home/abdul/CLionProjects/Compiler/examples/loop examples/mc91.f");
    Compiler c(st, "../warnings.txt");
    stringstream pleaseWork;
    c.compile(pleaseWork);
    cout << pleaseWork.str();
    return 0;
}