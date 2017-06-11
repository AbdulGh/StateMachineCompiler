#ifndef LEXER_H
#define LEXER_H

#include <fstream>
#include <string>
#include <unordered_map>
#include "Token.h"

class Lexer
{
private:
    std::ifstream infile;
    std::unordered_map<std::string, Token> resWords;
    void initResWords();
    int currentLine;

public:
    Lexer(std::string);
    ~Lexer();
    Token getNextToken();
    int getLine();
};


#endif
