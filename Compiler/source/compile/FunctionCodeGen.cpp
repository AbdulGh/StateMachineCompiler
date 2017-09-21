#include "FunctionCodeGen.h"

using namespace std;

FunctionCodeGen::FunctionCodeGen(VariableType rt, std::vector<VariableType> types, std::string in, ControlFlowGraph& c):
    returnType(rt), paramTypes(move(types)), ident(move(in)),
    currentStates(1), endedState(false), cfg(c)
    {currentNode = cfg.createNode("F_" + ident + "_0", this, false, false);}

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

VariableType FunctionCodeGen::getReturnType() const
{
    return returnType;
}

/*generation*/
void FunctionCodeGen::genNewState(std::string n)
{
    if (!endedState) throw "Unfinished state";
    currentNode = cfg.createNode(n, this, true, false);
    endedState = false;
}

void FunctionCodeGen::genEndState()
{
    if (endedState) throw "No state to end";
    currentNode->setInstructions(currentInstrs);
    currentInstrs.clear();
    endedState = true;
}

void FunctionCodeGen::genJump(std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_shared<JumpCommand>(s, linenum));
}

void FunctionCodeGen::genPrint(std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_shared<PrintCommand>(s, linenum));
}

void FunctionCodeGen::genConditionalJump(std::string state, std::string lh, Relations::Relop r, std::string rh, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_shared<JumpOnComparisonCommand>(state, lh, rh, r, linenum));
}

void FunctionCodeGen::genPop(std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_shared<PopCommand>(s, linenum));
}

void FunctionCodeGen::genReturn(int linenum)
{
    if (endedState) throw "No state to add to";
    if (lastNode == nullptr)
    {
        vector<shared_ptr<AbstractCommand>> returnCommand({make_shared<ReturnCommand>(-1)});
        lastNode =
                cfg.createNode("F_" + ident + "_fin", this, false, true);
        lastNode->setInstructions(returnCommand);
    }
    currentInstrs.push_back(make_shared<JumpCommand>(lastNode->getName(), linenum));
}


void FunctionCodeGen::genPush(std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_shared<PushCommand>(s, linenum));
}

void FunctionCodeGen::genInput(std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_shared<InputVarCommand>(s, linenum));
}

void FunctionCodeGen::genExpr(std::string lh, std::string t1, Op o, std::string t2, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_shared<EvaluateExprCommand>(lh, t1, o, t2, linenum));
}

void FunctionCodeGen::genVariableDecl(VariableType t, std::string n, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_shared<DeclareVarCommand>(t, n, linenum));
}

void FunctionCodeGen::addCommand(shared_ptr<AbstractCommand> ac)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(ac);
}

void FunctionCodeGen::genAssignment(std::string LHS, std::string RHS, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_shared<AssignVarCommand>(LHS, RHS, linenum));
}

const shared_ptr<CFGNode>& FunctionCodeGen::getLastNode()
{
    if (lastNode == nullptr)
    {
        vector<shared_ptr<AbstractCommand>> returnCommand({make_shared<ReturnCommand>(-1)});
        lastNode =
                cfg.createNode("F_" + ident + "_fin", this, false, true);
    }

    return lastNode;
}

void FunctionCodeGen::setLastNode(const shared_ptr<CFGNode> &lastNode)
{
    FunctionCodeGen::lastNode = lastNode;
}

const shared_ptr<CFGNode>& FunctionCodeGen::getFirstNode()
{
    if (firstNode == nullptr) firstNode = cfg.createNode("F_" + ident + "_0", this, true, false);
    return firstNode;
}

void FunctionCodeGen::setFirstNode(const shared_ptr<CFGNode> &firstNode)
{
    FunctionCodeGen::firstNode = firstNode;
}

const shared_ptr<CFGNode> &FunctionCodeGen::getCurrentNode() const
{
    return currentNode;
}



