#include <iostream>

#include "SymbolTable.h"

using namespace std;

SymbolTable::SymbolTable()
{
    currentMap = make_unique<SymbolTableMap>();
    scopeDepths.push_back(0);
    depth = 0;
}

void SymbolTable::pushScope()
{
    sTable.push_front(move(currentMap));
    currentMap = make_unique<SymbolTableMap>();
    depth += 1;
    if (depth == scopeDepths.size()) scopeDepths.push_back(0);
    else ++scopeDepths[depth];
}

void SymbolTable::popScope()
{
    if (depth-- == 0) throw runtime_error("Tried to pop base scope");
    currentMap.reset();
    currentMap = move(sTable.front());
    sTable.pop_front();
}

Identifier* SymbolTable::declare(VariableType type, std::string name, int lineNum)
{
    unique_ptr<Identifier> up = make_unique<Identifier>(name, type, lineNum, depth, scopeDepths[depth]);
    Identifier* idptr = up.get();
    currentMap->operator[](name) = move(up);
    return idptr;
}

bool SymbolTable::isDeclared(const string& name)
{
    return (findIdentifier(name) != nullptr);
}

bool SymbolTable::isInScope(const string& name)
{
    unordered_map<string, unique_ptr<Identifier>>::const_iterator it = currentMap->find(name);
    return it != currentMap->cend();
}

bool SymbolTable::isDefined(const string& name)
{
    if (Identifier* id = findIdentifier(name)) return id->isDefined();
    return false;
}

void SymbolTable::define(VariableType type, const string& name)
{
    if (Identifier* id = findIdentifier(name)) id->setDefined();
    throw runtime_error("Variable '" + name + "' not found");
}

Identifier* SymbolTable::findIdentifier(const string& name)
{
    SymbolTableMap::const_iterator it = currentMap->find(name);
    if (it != currentMap->cend()) return it->second.get();
    else
    {
        for (auto& scope: sTable)
        {
            SymbolTableMap::const_iterator it = scope->find(name);
            if (it != currentMap->cend()) return it->second.get();
        }
    }
    return nullptr;
}