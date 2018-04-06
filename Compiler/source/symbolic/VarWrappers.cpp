//
// Created by abdul on 19/03/18.
//

#include <memory>

#include "VarWrappers.h"
#include "SymbolicExecution.h"

//SVByName
GottenVarPtr<SymbolicDouble> SVByName::getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) const
{
    SymbolicDouble* foundsv = sef->symbolicVarSet->findVar(name);
    return GottenVarPtr<SymbolicDouble>(foundsv);
}

void SVByName::setSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble* sv)
{
    std::unique_ptr<SymbolicDouble> sv2 = sv->clone();
    sv2->setName(name);
    sef->symbolicVarSet->addVar(move(sv2));
}

void SVByName::setConstValue(SymbolicExecution::SymbolicExecutionFringe *sef, double d)
{
    auto var = sef->symbolicVarSet->findVar(name);
    if (!var) throw std::runtime_error("Undefined variable '" + name + "'");
    var->setConstValue(d);
}

void SVByName::nondet(SymbolicExecution::SymbolicExecutionFringe* sef)
{
    auto var = sef->symbolicVarSet->findVar(name);
    if (!var) throw std::runtime_error("Undefined variable '" + name + "'");
    var->nondet();
}

bool SVByName::check(SymbolicExecution::SymbolicExecutionFringe *sef) const
{
    auto var = sef->symbolicVarSet->findVar(name);
    return var != nullptr;
}

std::string SVByName::getFullName() const
{
    return name;
}

std::unique_ptr<VarWrapper> SVByName::clone() const
{
    return std::make_unique<SVByName>(name);
}

//SDByArrayIndex
GottenVarPtr<SymbolicDouble> SDByArrayIndex::getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) const
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    return GottenVarPtr<SymbolicDouble>(move(sa->operator[](index)));
}

bool SDByArrayIndex::check(SymbolicExecution::SymbolicExecutionFringe* sef) const
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr)
    {
        sef->error(Reporter::UNDECLARED_USE, "Array '" + name + "' undeclared");
        return false;
    }
    sa->checkIndex(index);
}

void SDByArrayIndex::setSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble* sd)
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    sa->set(index, sd);
}

void SDByArrayIndex::setConstValue(SymbolicExecution::SymbolicExecutionFringe *sef, double d)
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    sa->set(index, d);
}

void SDByArrayIndex::nondet(SymbolicExecution::SymbolicExecutionFringe* sef)
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    sa->nondet(index);
}

std::unique_ptr<VarWrapper> SDByArrayIndex::clone() const
{
    return std::make_unique<SDByArrayIndex>(name, index);
}

//SDByIndexVar
GottenVarPtr<SymbolicDouble> SDByIndexVar::getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) const
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    auto sv = index->getSymbolicDouble(sef);
    return GottenVarPtr<SymbolicDouble>(sa->operator[](sv.get()));
}

bool SDByIndexVar::check(SymbolicExecution::SymbolicExecutionFringe* sef) const
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr)
    {
        sef->error(Reporter::UNDECLARED_USE, "Array '" + name + "' undeclared");
        return false;
    }

    auto sv = index->getSymbolicDouble(sef);
    if (sv->isDetermined()) return sa->checkIndex(sv->getConstValue());
    else return sa->checkBounds(sv->getLowerBound(), sv->getUpperBound());
}

void SDByIndexVar::setSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble* sd)
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    SymbolicDouble* ind = index->getSymbolicDouble(sef).get();
    sa->set(ind, sd);
}

void SDByIndexVar::setConstValue(SymbolicExecution::SymbolicExecutionFringe *sef, double d)
{

    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    SymbolicDouble val("val", sef->reporter);
    val.setConstValue(d);
    sa->set(index->getSymbolicDouble(sef).get(), &val);
}

void SDByIndexVar::nondet(SymbolicExecution::SymbolicExecutionFringe* sef)
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    sa->nondet(index->getSymbolicDouble(sef).get());
}

std::unique_ptr<VarWrapper> SDByIndexVar::clone() const
{
    return std::make_unique<SDByIndexVar>(name, index->clone());
}