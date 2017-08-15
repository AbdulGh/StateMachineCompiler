//
// Created by abdul on 10/08/17.
//
#include "Command.h"
#include "symbolic/SymbolicExecution.h"

using namespace std;

void AbstractCommand::acceptSymbolicExecution(SymbolicExecution::SymbolicExecutionFringe &svs)
{
    return;
}

void InputVarCommand::acceptSymbolicExecution(SymbolicExecution::SymbolicExecutionFringe& svs)
{
    VarPointer found = svs.symbolicDoubleSet.findVar(getData());
    if (found == nullptr) return; //string - compiler (probably) cannot produce code w/ undeclared variables
    found->userInput();
}

void PushCommand::acceptSymbolicExecution(SymbolicExecution::SymbolicExecutionFringe &svs)
{

}

void PopCommand::acceptSymbolicExecution(SymbolicExecution::SymbolicExecutionFringe &svs)
{

}

void AssignVarCommand::acceptSymbolicExecution(SymbolicExecution::SymbolicExecutionFringe &svs)
{

}

void EvaluateExprCommand::acceptSymbolicExecution(SymbolicExecution::SymbolicExecutionFringe &svs)
{

}

void DeclareVarCommand::acceptSymbolicExecution(SymbolicExecution::SymbolicExecutionFringe &svs)
{

}

