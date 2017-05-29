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
    int lastPos;
    int currentLine;

public:
    Lexer(std::string);
    ~Lexer();
    Token getNextToken();
    void unget();
    int getLine();
};


#endif
