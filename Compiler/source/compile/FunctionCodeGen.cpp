#include "FunctionCodeGen.h"

using namespace std;

FunctionCodeGen::FunctionCodeGen(VariableType rt, std::vector<VariableType> types, std::string in, ControlFlowGraph& c):
    returnType(rt), paramTypes(types), ident(in),
    currentStates(1), currentStateName("F_" + ident + "_0"),
    endedState(false), cfg(c){}

bool FunctionCodeGen::checkTypes(std::vector<VariableType>& potential)
{
    return ((potential.size() == paramTypes.size()) && paramTypes == potential);
}

const string FunctionCodeGen::newStateName()
{
    return "F_" + ident + "_" + to_string(currentStates++);
}

const string& FunctionCodeGen::getIdentifier() const
{
    return ident;
}

bool FunctionCodeGen::isOfType(VariableType c)
{
    return (c == returnType);
}

/*
string FunctionCodeGen::getSource()
{
    if (currentInstrs.size() != 0) throw "Not done";

    std::stringstream outs;
    for (State s : finStates)
    {
        outs << s.getName() << endl;
        for (auto& i: s.getInstructions()) outs << i->translation();
        outs << "end" << endl << endl;
    }
    return outs.str();
}*/

VariableType FunctionCodeGen::getReturnType() const
{
    return returnType;
}

/*generation*/

void FunctionCodeGen::genNewState(std::string n)
{
    if (!endedState) throw "Unfinished state";
    currentStateName = n;
    endedState = false;
}

void FunctionCodeGen::genEndState()
{
    if (endedState) throw "No state to end";
    cfg.addNode(currentStateName, currentInstrs);
    currentInstrs.clear();
    endedState = true;
}

void FunctionCodeGen::genJump(std::string s)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new JumpCommand(s)));
}

void FunctionCodeGen::genPrint(std::string s)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new PrintCommand(s)));
}

void FunctionCodeGen::genConditionalJump(std::string state, std::string lh, Relop r, std::string rh)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new JumpOnComparisonCommand(state, lh, rh, r)));
}

void FunctionCodeGen::genPop(std::string s)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new PopCommand(s)));
}

void FunctionCodeGen::genReturn()
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new ReturnCommand()));
}


void FunctionCodeGen::genPush(std::string s)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new PushCommand(s)));
}

void FunctionCodeGen::genInput(std::string s)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new InputVarCommand(s)));
}

void FunctionCodeGen::genExpr(std::string lh, std::string t1, Op o, std::string t2)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new EvaluateExprCommand(lh, t1, o, t2)));
}

void FunctionCodeGen::genVariableDecl(VariableType t, std::string n)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new DeclareVariableCommand(t, n)));
}

void FunctionCodeGen::addCommand(shared_ptr<AbstractCommand> ac)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(ac);
}

void FunctionCodeGen::genAssignment(std::string LHS, std::string RHS)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new AssignVarCommand(LHS, RHS)));
}
