#include <iostream>

#include "SymbolTable.h"

using namespace std;

SymbolTable::SymbolTable()
{
    currentMap = make_shared<SymbolTableMap>();
    scopeDepths.push_back(0);
    depth = 0;
}

void SymbolTable::pushScope()
{
    sTable.push_front(currentMap);
    currentMap = make_shared<SymbolTableMap>();
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
    shared_ptr<Identifier> sp = make_shared<Identifier>(name, type, lineNum, depth, scopeDepths[depth]);
    currentMap->operator[](name) = sp;
    return sp;
}

bool SymbolTable::isDeclared(string name)
{
    return (findIdentifier(name) != nullptr);
}

bool SymbolTable::isInScope(string name)
{
    unordered_map<string, shared_ptr<Identifier>>::const_iterator it = currentMap->find(name);
    return it != currentMap->cend();
}

bool SymbolTable::isDefined(string name)
{
    if (shared_ptr<Identifier> id = findIdentifier(name)) return id->isDefined();
    return false;
}

bool SymbolTable::define(VariableType type, string name)
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
    SymbolTableMap::const_iterator it = currentMap->find(name);
    if (it != currentMap->cend()) return it->second;
    else
    {
        for (auto scope: sTable)
        {
            SymbolTableMap::const_iterator it = scope->find(name);
            if (it != currentMap->cend()) return it->second;
        }
    }
    return nullptr;
}