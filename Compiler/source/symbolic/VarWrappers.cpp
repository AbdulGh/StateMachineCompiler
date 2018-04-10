//
// Created by abdul on 19/03/18.
//

#include <memory>

#include "VarWrappers.h"
#include "SymbolicExecution.h"

//SDByName
GottenVarPtr<SymbolicDouble> SDByName::getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, int linenum) const
{
    SymbolicDouble* foundsv = sef->symbolicVarSet->findVar(name);
    return GottenVarPtr<SymbolicDouble>(foundsv);
}

void SDByName::setSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble* sd, int linenum)
{
    std::unique_ptr<SymbolicDouble> sd2 = sd->clone();
    sd2->setName(name);
    sd2->define();
    sef->symbolicVarSet->addVar(move(sd2));
}

void SDByName::setConstValue(SymbolicExecution::SymbolicExecutionFringe* sef, double d, int linenum)
{
    auto var = sef->symbolicVarSet->findVar(name);
    if (!var) throw std::runtime_error("Undefined variable '" + name + "'");
    var->setConstValue(d);
}

void SDByName::nondet(SymbolicExecution::SymbolicExecutionFringe* sef, int linenum)
{
    auto var = sef->symbolicVarSet->findVar(name);
    if (!var) throw std::runtime_error("Undefined variable '" + name + "'");
    var->nondet();
}

bool SDByName::check(SymbolicExecution::SymbolicExecutionFringe* sef, int linenum) const
{
    auto var = sef->symbolicVarSet->findVar(name);
    return var != nullptr;
}

std::string SDByName::getFullName() const
{
    return name;
}

std::unique_ptr<VarWrapper> SDByName::clone() const
{
    return std::make_unique<SDByName>(name);
}

//SDByArrayIndex
GottenVarPtr<SymbolicDouble> SDByArrayIndex::getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, int linenum) const
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    return GottenVarPtr<SymbolicDouble>(move(sa->get(index, linenum)));
}

bool SDByArrayIndex::check(SymbolicExecution::SymbolicExecutionFringe* sef, int linenum) const
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr)
    {
        sef->error(Reporter::UNDECLARED_USE, "Array '" + name + "' undeclared");
        return false;
    }
    sa->checkIndex(index, linenum);
}

void SDByArrayIndex::setSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble* sd, int linenum)
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    sa->set(index, sd, linenum);
}

void SDByArrayIndex::setConstValue(SymbolicExecution::SymbolicExecutionFringe* sef, double d, int linenum)
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    sa->set(index, d, linenum);
}

void SDByArrayIndex::nondet(SymbolicExecution::SymbolicExecutionFringe* sef, int linenum)
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    sa->nondet(index, linenum);
}

std::unique_ptr<VarWrapper> SDByArrayIndex::clone() const
{
    return std::make_unique<SDByArrayIndex>(name, index);
}

//SDByIndexVar
GottenVarPtr<SymbolicDouble> SDByIndexVar::getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, int linenum) const
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    auto sd = index->getSymbolicDouble(sef, linenum);
    return GottenVarPtr<SymbolicDouble>(sa->get(sd.get(), linenum));
}

bool SDByIndexVar::check(SymbolicExecution::SymbolicExecutionFringe* sef, int linenum) const
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr)
    {
        sef->error(Reporter::UNDECLARED_USE, "Array '" + name + "' undeclared");
        return false;
    }

    auto sd = index->getSymbolicDouble(sef, linenum);
    if (sd->isDetermined()) return sa->checkIndex(sd->getConstValue(), linenum);
    else return sa->checkBounds(sd->getLowerBound(), sd->getUpperBound(), linenum);
}

void SDByIndexVar::setSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicDouble* sd, int linenum)
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    SymbolicDouble* ind = index->getSymbolicDouble(sef, linenum).get();
    sa->set(ind, sd, linenum);
}

void SDByIndexVar::setConstValue(SymbolicExecution::SymbolicExecutionFringe* sef, double d, int linenum)
{

    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    SymbolicDouble val("val", sef->reporter);
    val.setConstValue(d);
    sa->set(index->getSymbolicDouble(sef, linenum).get(), &val, linenum);
}

void SDByIndexVar::nondet(SymbolicExecution::SymbolicExecutionFringe* sef, int linenum)
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    sa->nondet(index->getSymbolicDouble(sef, linenum).get(), linenum);
}

std::unique_ptr<VarWrapper> SDByIndexVar::clone() const
{
    return std::make_unique<SDByIndexVar>(name, index->clone());
}