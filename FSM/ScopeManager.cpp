#include "ScopeManager.h"
#include <iostream>

using namespace std;
using sPtrType = unordered_map<string, shared_ptr<Variable>>;

ScopeManager::ScopeManager()
{
    currentMap = shared_ptr<sPtrType>(new unordered_map<string, shared_ptr<Variable>>);
}

void ScopeManager::pushScope()
{
    sTable.push_front(currentMap);
    currentMap = shared_ptr<sPtrType>(new unordered_map<string, shared_ptr<Variable>>);
}

void ScopeManager::popScope()
{
    currentMap.reset();
    currentMap = sTable.front();
    sTable.pop_front();
}

void ScopeManager::declare(string name, Variable::Type type)
{
    shared_ptr<Variable> sp;
    switch (type)
    {
        case Variable::Type::INT:
            sp = shared_ptr<Variable>(new Variable(name, 0));

        case Variable::Type::STRING:
            sp = shared_ptr<Variable>(new Variable(name, ""));

        case Variable::Type::DOUBLE:
            sp = shared_ptr<Variable>(new Variable(name, 0.0));

        default:
            throw "Strange data effect encountered.";
    }

    currentMap->operator[](name) = sp;
}

shared_ptr<Variable> ScopeManager::findVariable(std::string name)
{
    if (currentMap->count(name)) return currentMap->at(name);
    else
    {
        for (auto scope: sTable)
        {
            if (scope->count(name)) return scope->at(name);
        }
    }
    throw std::runtime_error("Variable '" + name + "' undefined");
}

void ScopeManager::set(std::string name, int i)
{
    findVariable(name)->setData(i);
};

void ScopeManager::set(std::string name, double d)
{
    findVariable(name)->setData(d);
};

void ScopeManager::set(std::string name, std::string str)
{
    findVariable(name)->setData(str);
};

/*
;bool ScopeManager::isDeclared(std::string name, Variable::Type effect)
{
    if (shared_ptr<Variable> id = findVariable(name))
    {
        return id->getType() == effect;
    }
    return false;
}

bool ScopeManager::isDefined(std::string name, Variable::Type effect)
{
    if (shared_ptr<Variable> id = findVariable(name))
    {
        return id->getType() == effect && id->isDefined();
    }
    return false;
}

bool ScopeManager::define(std::string name)
{
    if (shared_ptr<Variable> id = findVariable(name))
    {
        id->setDefined();
        return true;
    }
    return false;
}
 */