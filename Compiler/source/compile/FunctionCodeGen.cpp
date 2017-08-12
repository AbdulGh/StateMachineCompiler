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

const string FunctionCodeGen::getLastStateName() const
{
    return "F_" + ident + "_" + to_string(currentStates - 1);
}

const string& FunctionCodeGen::getIdentifier() const
{
    return ident;
}

bool FunctionCodeGen::isOfType(VariableType c)
{
    return (c == returnType);
}

VariableType FunctionCodeGen::getReturnType() const
{
    return returnType;
}

/*generation*/
//if this is not of the form F_ident_n everything gets broken
//todo make this based only on the number
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

void FunctionCodeGen::genJump(std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new JumpCommand(s, linenum)));
}

void FunctionCodeGen::genPrint(std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new PrintCommand(s, linenum)));
}

void FunctionCodeGen::genConditionalJump(std::string state, std::string lh, Relop r, std::string rh, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new JumpOnComparisonCommand(state, lh, rh, r, linenum)));
}

void FunctionCodeGen::genPop(std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new PopCommand(s, linenum)));
}

void FunctionCodeGen::genReturn(int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new ReturnCommand(linenum)));
}


void FunctionCodeGen::genPush(std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new PushCommand(s, linenum)));
}

void FunctionCodeGen::genInput(std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new InputVarCommand(s, linenum)));
}

void FunctionCodeGen::genExpr(std::string lh, std::string t1, Op o, std::string t2, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new EvaluateExprCommand(lh, t1, o, t2, linenum)));
}

void FunctionCodeGen::genVariableDecl(VariableType t, std::string n, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new DeclareVarCommand(t, n, linenum)));
}

void FunctionCodeGen::addCommand(shared_ptr<AbstractCommand> ac)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(ac);
}

void FunctionCodeGen::genAssignment(std::string LHS, std::string RHS, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(shared_ptr<AbstractCommand>(new AssignVarCommand(LHS, RHS, linenum)));
}
