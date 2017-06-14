#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <forward_list>
#include <unordered_map>
#include <string>
#include <memory>

#include "Token.h"

class Identifier
{
private:
    std::string lexeme;
    int lineNum;
    VariableType type;
    bool defined;

public:
    Identifier(std::string identifier, VariableType datatype, int line) :
            lexeme(identifier),
            lineNum(line),
            type(datatype),
            defined(false) {}

    const std::string &getLexeme() const
    {
        return lexeme;
    }

    int getLineNum() const
    {
        return lineNum;
    }

    void setDefined()
    {
        defined = true;
    }

    VariableType getType() const
    {
        return type;
    }

    bool isDefined() const
    {
        return defined;
    }
};

class SymbolTable
{
private:
    std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Identifier>>> currentMap;
    std::forward_list<std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Identifier>>>> sTable;

public:
    SymbolTable();
    std::shared_ptr<Identifier> findIdentifier(std::string name);
    void pushScope();
    void popScope();
    void declare(std::string name, VariableType type, int lineNum);
    bool define(std::string name);
    bool isDeclared(std::string name, VariableType type);
    bool isDefined(std::string name, VariableType type);
};


#endif
