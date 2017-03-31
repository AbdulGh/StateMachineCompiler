#include <iostream>
#include "Token.h"
#include "SymbolTable.h"
#include "Lexer.h"

using namespace std;

int main()
{
    Lexer lexer("../misc/test.f");
    Token next;

    while (next.type != END)
    {
        next = lexer.getNextToken();
        cout << next << " ";
    }
    return 0;
}