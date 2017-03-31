#include "SymbolTable.h"
#include <iostream>

using namespace std;
using sPtrType = unordered_map<string, shared_ptr<Identifier>>;

SymbolTable::SymbolTable()
{
    currentMap = shared_ptr<sPtrType>(new unordered_map<string, shared_ptr<Identifier>>);
}

void SymbolTable::pushScope()
{
    sTable.push_front(currentMap);
    currentMap = shared_ptr<sPtrType>(new unordered_map<string, shared_ptr<Identifier>>);
}

void SymbolTable::popScope()
{
    currentMap.reset();
    currentMap = sTable.front();
    sTable.pop_front();
}

void SymbolTable::declare(string name, VariableType type, int lineNum)
{
    shared_ptr<Identifier> sp = shared_ptr<Identifier>(new Identifier(name, type, lineNum));
    currentMap->operator[](name) = sp;
}

bool SymbolTable::isDeclared(std::string name, VariableType type)
{
    if (shared_ptr<Identifier> id = findIdentifier(name))
    {
        return id->getType() == type;
    }
    return false;
}

bool SymbolTable::isDefined(std::string name, VariableType type)
{
    if (shared_ptr<Identifier> id = findIdentifier(name))
    {
        return id->getType() == type && id->isDefined();
    }
    return false;
}

bool SymbolTable::define(std::string name)
{
    if (shared_ptr<Identifier> id = findIdentifier(name))
    {
        id->setDefined();
        return true;
    }
    return false;
}

shared_ptr<Identifier> SymbolTable::findIdentifier(std::string name)
{
    if (currentMap->count(name)) return currentMap->at(name);
    else
    {
        for (auto scope: sTable)
        {
            if (scope->count(name)) return scope->at(name);
        }
    }
    return nullptr;
}