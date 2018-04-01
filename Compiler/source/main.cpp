#include <iostream>
#include "compile/Compiler.h"
#include "compile/Lexer.h"
#include "symbolic/SymbolicArray.h"
#include <limits>
using namespace std;

int main()
{
    Lexer lexer;
    vector<Token> st = lexer.tokenize("/home/abdul/CLionProjects/Compiler/examples/loop examples/bubblesort.f");
    Compiler c(st, "../warnings.txt");
    stringstream pleaseWork;
    c.compile(pleaseWork);
    cout << pleaseWork.str();
    return 0;
}