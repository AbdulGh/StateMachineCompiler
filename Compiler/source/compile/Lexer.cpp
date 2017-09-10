#include "Lexer.h"
#include <iostream>

using namespace std;

Lexer::Lexer()
{
    currentLine = 1;
    lastChar = 0;

    resWords["if"] = Token(IF);
    resWords["while"] = Token(WHILE);
    resWords["function"] = Token(FUNCTION);
    resWords["string"] = Token(STRING);
    resWords["double"] = Token(DOUBLE);
    resWords["call"] = Token(CALL);
    resWords["input"] = Token(INPUT);
    resWords["print"] = Token(PRINT);
    resWords["endif"] = Token(ENDIF);
    resWords["return"] = Token(RETURN);
    resWords["void"] = Token(VOID);
    resWords["else"] = Token(ELSE);
    //resWords["even"] = Token(RETURN);
    //resWords["done"] = Token(DONE);
    //resWords["then"] = Token(THEN);
}

vector<Token> Lexer::tokenize(string str)
{
    infile.open(str);
    if (!infile.good()) throw runtime_error("Could not open filename '" + str + "' for lexing.");

    vector<Token> stream;

    while(infile)
    {
        Token t = parseToken();
        t.setLine(currentLine);
        stream.push_back(t);
        if (t.type == END) break;
    }

    infile.close();
    return stream;
}

char Lexer::getChar()
{
    char c;
    if(infile.get(c))
    {
        if (c == '\n') currentLine++;
        lastChar = c;
        return c;
    }
    else return 0;
}

void Lexer::unget()
{
    if (lastChar == 0) throw runtime_error("Cannot unget twice");
    if (lastChar == '\n') currentLine--;
    lastChar = 0;
    infile.unget();
}

Token Lexer::parseToken()
{
    char c;
    if (!(c = getChar())) return Token(END);
    while (isspace(c) && (c = getChar()));
    if (c == 0) return Token(END);

    switch(c)
    {
        case '{': return Token(LBRACE);
        case '}': return Token(RBRACE);
        case ';': return Token(SEMIC);
        case '(': return Token(LPAREN);
        case ')': return Token(RPAREN);
        case '+': return Token(PLUS);
        case '-': return Token(MINUS);
        case '/': return Token(DIV);
        case '*': return Token(MULT);
        case '%': return Token(MOD);
        case ',': return Token(COMMA);
        case '|':
            if ((c = getChar()) && c == '|') return Token(COMPOR);
            else
            {
                unget();
                return Token(OR);
            }
        case '&':
            if ((c = getChar()) && c == '&') return Token(COMPAND);
            else
            {
                unget();
                return Token(AND);
            }
        case '=':
            if ((c = getChar()) && c == '=') return Token(Relations::EQ);
            else
            {
                unget();
                return Token(ASSIGN);
            }
        case '<':
            if ((c = getChar()) && c == '=') return Token(Relations::LE);
            else
            {
                unget();
                return Token(Relations::LT);
            }
        case '>':
            if ((c = getChar()) && c == '=') return Token(Relations::GE);
            else
            {
                unget();
                return Token(Relations::GT);
            }
        case '!':
            if ((c = getChar()) && c == '=') return Token(Relations::NE);
            else
            {
                unget();
                return Token(NOT);
            }
        case '"': case '\'':
        {
            char delim = c;
            string lit = "";
            while ((c = getChar()) && c != delim) lit += c;
            return Token(STRINGLIT, lit);
        }
        default: //todo handle numbers by checking c
            string str(1, c);
            while ((c = getChar()) && isalnum(c)) str += c;
            if (!infile.eof()) unget();

            if (str == "") return parseToken();

            try //number
            {
                stod(str);
                return Token(NUMBER, str);
            }
            catch(invalid_argument e)
            {
                unordered_map<string, Token>::const_iterator found = resWords.find(str);
                if (found == resWords.end()) return Token(IDENT, str);
                else return found->second;
            }
    }
}