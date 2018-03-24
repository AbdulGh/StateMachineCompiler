//
// Created by abdul on 19/03/18.
//

#include "VarWrappers.h"
#include "SymbolicExecution.h"

//SVByName
GottenVarPtr<SymbolicVariable> SVByName::getSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef) const
{
    SymbolicVariable* foundsv = sef->symbolicVarSet->findVar(name);
    return GottenVarPtr<SymbolicVariable>(foundsv);
}

GottenVarPtr<SymbolicDouble> SVByName::getSymbolicDouble(SymbolicExecution::SymbolicExecutionFringe* sef) const
{
    SymbolicVariable* sv = sef->symbolicVarSet->findVar(name);
    if (sv == nullptr) throw std::runtime_error("Var '" + name + "' undeclared");
    if (sv->getType() != DOUBLE) throw std::runtime_error("wrong type");
    SymbolicDouble* sd = static_cast<SymbolicDouble*>(sv);

    return GottenVarPtr<SymbolicDouble>(sd);
}

VariableType SVByName::getVariableType(SymbolicExecution::SymbolicExecutionFringe* sef) const
{
    SymbolicVariable* sv = sef->symbolicVarSet->findVar(name);
    if (sv == nullptr) throw std::runtime_error("Var '" + name + "' undeclared");
    return sv->getType();
}

void SVByName::setSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicVariable* sv)
{
    sv->setName(name);
    sef->symbolicVarSet->addVar(sv->clone());
}

void SVByName::setConstValue(SymbolicExecution::SymbolicExecutionFringe* sef, std::string sv)
{
    auto var = sef->symbolicVarSet->findVar(name);
    if (!var) throw std::runtime_error("Undefined variable '" + name + "'");
    var->setConstValue(sv);
}

void SVByName::nondet(SymbolicExecution::SymbolicExecutionFringe* sef)
{
    auto var = sef->symbolicVarSet->findVar(name);
    if (!var) throw std::runtime_error("Undefined variable '" + name + "'");
    var->userInput();
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
    return GottenVarPtr<SymbolicDouble>(move(sa->operator[](index))); //WHY operator[]???!
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

void SDByArrayIndex::setSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicVariable* sv)
{
    if (sv->getType() != DOUBLE) throw "should be double";
    SymbolicDouble* sd = static_cast<SymbolicDouble*>(sv);
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    sa->set(index, sd);
}

void SDByArrayIndex::setConstValue(SymbolicExecution::SymbolicExecutionFringe* sef, std::string sv)
{
    double d;
    try {d = std::stod(sv);}
    catch (std::invalid_argument&) {throw std::runtime_error("Arrays hold only doubles");}
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

VariableType SDByArrayIndex::getVariableType(SymbolicExecution::SymbolicExecutionFringe* sef) const
{
    return DOUBLE;
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
    if (sv->getType() != DOUBLE) throw std::runtime_error("wrong type");
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
    if (sv->isDetermined()) return sa->checkIndex(sv->getTConstValue());
    else return sa->checkBounds(sv->getTLowerBound(), sv->getTUpperBound());
}

void SDByIndexVar::setSymbolicVariable(SymbolicExecution::SymbolicExecutionFringe* sef, SymbolicVariable* sv)
{
    if (sv->getType() != DOUBLE) throw "should be double";
    SymbolicDouble* sd = static_cast<SymbolicDouble*>(sv);
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    SymbolicDouble* ind = index->getSymbolicDouble(sef).get();
    sa->set(ind, sd);
}

void SDByIndexVar::setConstValue(SymbolicExecution::SymbolicExecutionFringe* sef, std::string sv)
{
    double d;
    try {d = std::stod(sv);}
    catch (std::invalid_argument&) {throw std::runtime_error("Arrays hold only doubles");}
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    SymbolicDouble val("val", sef->reporter);
    val.setTConstValue(d);
    sa->set(index->getSymbolicDouble(sef).get(), &val);
}

void SDByIndexVar::nondet(SymbolicExecution::SymbolicExecutionFringe* sef)
{
    SymbolicArray* sa = sef->symbolicVarSet->findArray(name);
    if (sa == nullptr) throw std::runtime_error("Array '" + name + "' undeclared");
    sa->nondet(index->getSymbolicDouble(sef).get());
}

VariableType SDByIndexVar::getVariableType(SymbolicExecution::SymbolicExecutionFringe* sef) const
{
    return DOUBLE;
}

std::unique_ptr<VarWrapper> SDByIndexVar::clone() const
{
    return std::make_unique<SDByIndexVar>(name, index->clone());
}