#ifndef LEXER_H
#define LEXER_H

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Token.h"

class Lexer
{
private:
    std::ifstream infile;
    std::unordered_map<std::string, Token> resWords;

    char lastChar;
    int currentLine;

    char getChar();
    void unget();
    Token parseToken();

public:
    Lexer();
    std::vector<Token> tokenize(std::string);
};


#endif
