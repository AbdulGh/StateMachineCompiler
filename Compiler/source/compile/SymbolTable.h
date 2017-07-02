#ifndef PROJECT_SYMBOLTABLE_H
#define PROJECT_SYMBOLTABLE_H

#include <forward_list>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Token.h"

class Identifier
{
private:
    std::string lexeme;
    std::string uniqueID;
    unsigned int lineNum;
    VariableType type;
    bool defined;
    //unsigned int scopeDepthLevel;
    //unsigned int scopeNumber;

public:
    Identifier(std::string identifier, VariableType datatype, unsigned int line,unsigned int depth,unsigned int scopenum) :
            lexeme(identifier),
            lineNum(line),
            type(datatype),
            defined(false),
            uniqueID("_" + std::to_string(scopenum) + "_" + std::to_string(depth) + "_" + lexeme){}

    const std::string &getLexeme() const
    {
        return lexeme;
    }

    const std::string &getUniqueID() const
    {
        return uniqueID;
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
    //used for generating identifiers
    std::vector<unsigned int> scopeDepths;
    unsigned int depth;

public:
    SymbolTable();
    std::shared_ptr<Identifier> findIdentifier(std::string name);
    void pushScope();
    void popScope();
    std::shared_ptr<Identifier> declare(std::string name, VariableType type, unsigned int lineNum);
    bool define(std::string name);
    bool isDeclared(std::string name, VariableType type);
    bool isDefined(std::string name, VariableType type);
};


#endif
