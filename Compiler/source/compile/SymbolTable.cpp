#include <iostream>

#include "SymbolTable.h"

using namespace std;
using sPtrType = unordered_map<string, shared_ptr<Identifier>>;

//todo clear up ambiguous var stuff

SymbolTable::SymbolTable()
{
    currentMap = shared_ptr<sPtrType>(new unordered_map<string, shared_ptr<Identifier>>);
    scopeDepths.push_back(0);
    depth = 0;
}

void SymbolTable::pushScope()
{
    sTable.push_front(currentMap);
    currentMap = shared_ptr<sPtrType>(new unordered_map<string, shared_ptr<Identifier>>);
    depth += 1;
    if (depth == scopeDepths.size()) scopeDepths.push_back(0);
    else scopeDepths[depth]++;
}

void SymbolTable::popScope()
{
    if (depth-- == 0) throw runtime_error("Tried to pop base scope");
    currentMap.reset();
    currentMap = sTable.front();
    sTable.pop_front();
}

shared_ptr<Identifier> SymbolTable::declare(string name, VariableType type, unsigned int lineNum)
{
    shared_ptr<Identifier> sp = shared_ptr<Identifier>(new Identifier(name, type, lineNum, depth, scopeDepths[depth]));
    currentMap->operator[](name) = sp;
    return sp;
}

bool SymbolTable::isDeclared(string name, VariableType type)
{
    if (shared_ptr<Identifier> id = findIdentifier(name))
    {
        return id->getType() == type;
    }
    return false;
}

bool SymbolTable::isInScope(string name, VariableType type)
{
    unordered_map<string, shared_ptr<Identifier>>::const_iterator it = currentMap->find(name);
    return it != currentMap->cend();
}

bool SymbolTable::isDefined(string name, VariableType type)
{
    if (shared_ptr<Identifier> id = findIdentifier(name))
    {
        return id->getType() == type && id->isDefined();
    }
    return false;
}

bool SymbolTable::define(string name)
{
    if (shared_ptr<Identifier> id = findIdentifier(name))
    {
        id->setDefined();
        return true;
    }
    return false;
}

shared_ptr<Identifier> SymbolTable::findIdentifier(string name)
{
    unordered_map<string, shared_ptr<Identifier>>::const_iterator it = currentMap->find(name);
    if (it != currentMap->cend()) return it->second;
    else
    {
        for (auto scope: sTable)
        {
            unordered_map<string, shared_ptr<Identifier>>::const_iterator it = scope->find(name);
            if (it != currentMap->cend()) return it->second;
        }
    }
    return nullptr;
}