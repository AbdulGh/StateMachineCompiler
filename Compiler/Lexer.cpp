#include "Lexer.h"

using namespace std;

Lexer::Lexer(string str)
{
    infile.open(str);
    if (!infile.good()) throw runtime_error("Could not open filename '" + str + "' for lexing.");
    initResWords();
}

Lexer::~Lexer()
{
    infile.close();
}

void Lexer::initResWords()
{
    resWords["if"] = Token(IF);
    resWords["while"] = Token(WHILE);
    resWords["function"] = Token(FUNCTION);
    resWords["string"] = Token(STRING);
    resWords["int"] = Token(INT);
    resWords["double"] = Token(DOUBLE);
    resWords["call"] = Token(CALL);
    resWords["even"] = Token(EVEN);
    resWords["even"] = Token(RETURN);
    resWords["input"] = Token(INPUT);
    resWords["print"] = Token(PRINT);
}

Token Lexer::getNextToken()
{
    if (infile.eof()) return Token(END);

    char c;
    infile.get(c);

    while (isspace(c) && !infile.eof()) infile.get(c);

    switch(c)
    {
        case '{': return Token(LBRACE);
        case '}': return Token(RBRACE);
        case ';': return Token(SEMIC);
        case '(': return Token(LPAREN);
        case ')': return Token(RPAREN);
        case '"': return Token(QUOTE); //todo other quote
        case '+': return Token(PLUS);
        case '-': return Token(MINUS);
        case '/': return Token(DIV);
        case '*': return Token(MULT);
        case '=':
            infile.get(c);
            if (c == '=') return Token(EQ);
            else
            {
                infile.unget();
                return Token(ASSIGN);
            }
        case '<':
            infile.get(c);
            if (c == '=') return Token(LE);
            else
            {
                infile.unget();
                return Token(LT);
            }
        case '>':
            infile.get(c);
            if (c == '=') return Token(GE);
            else
            {
                infile.unget();
                return Token(GT);
            }
        case '!':
            infile.get(c);
            if (c == '=') return Token(NE);
            else
            {
                infile.unget();
                return Token(NOT);
            }
        default:
            string str = "";
            while (!isspace(c))
            {
                str += c;
                infile.get(c);
            }
            unordered_map<string, Token>::const_iterator found = resWords.find(str);
            if (found == resWords.end()) return Token(IDENT);
            else return found->second;
    }
}
