#include <iostream>
#include "compile/Compiler.h"
#include "symbolic/SymbolicVarWrappers.h"

using namespace std;

int main()
{
    string willyBumBum = "willy[bum[bum]]";
    auto x = parseAccess(willyBumBum);
    return 0;

    Lexer lexer;
    vector<Token> st = lexer.tokenize("/home/abdul/CLionProjects/Compiler/examples/array examples/basictest.f");
    Compiler c(st, "../warnings.txt");
    stringstream pleaseWork;
    c.compile(pleaseWork);
    cout << pleaseWork.str();
    return 0;
}