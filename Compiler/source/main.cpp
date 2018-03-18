#include <iostream>
#include "compile/Compiler.h"
#include "Command.h"

using namespace std;

//todo convert thrown strings to runtime_errors
int main()
{
    Lexer lexer;
    vector<Token> st = lexer.tokenize("/home/abdul/CLionProjects/Compiler/examples/array examples/basictest.f");
    Compiler c(st, "../warnings.txt");
    stringstream pleaseWork;
    c.compile(pleaseWork);
    cout << pleaseWork.str();
    return 0;
}