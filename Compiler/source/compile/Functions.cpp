#include <stack>
#include <algorithm>

#include "Functions.h"

using namespace std;

FunctionSymbol::FunctionSymbol(VariableType rt, std::vector<VariableType> types, std::string id, std::string p, ControlFlowGraph& c):
    returnType(rt), paramTypes(move(types)), ident(move(id)), prefix(move(p)),
    currentStates(1), endedState(false), cfg(c), lastNode{nullptr}
    {currentNode = cfg.createNode(prefix + "0", false, false, this); firstNode = currentNode;}


//assumes the entire function is reachable from the first node (most unreachable parts will be removed by symbolic execution)
void FunctionSymbol::giveNodesTo(FunctionSymbol* to)
{
    if (to->getPrefix() == getPrefix()) return;
    for (auto& pair : cfg.getCurrentNodes())
    {
        if (pair.second->getParentFunction()->getPrefix() == getPrefix())
        {
            pair.second->setParentFunction(to);
        }
    }
    to->setLastNode(getLastNode());
}

CFGNode* FunctionSymbol::getLastNode()
{
    if (lastNode == nullptr) lastNode = cfg.createNode(prefix + "fin", false, true, this);
    return lastNode;
}

void FunctionSymbol::setLastNode(CFGNode* ln)
{
    if (ln->getParentFunction()->getPrefix() != getPrefix()) throw "not my node";

    if (lastNode != nullptr)
    {
        if (lastNode->getName() == lastNode->getParentGraph().getLast()->getName())
        {
            lastNode->getParentGraph().setLast(ln->getName());
        }
        lastNode->setLast(false);
        for (const auto& ret : returnTo) ret->removeParent(lastNode);
    }
    lastNode = ln;
    lastNode->setLast();
    for (const auto& ret : returnTo) ret->addParent(lastNode);
}

CFGNode* FunctionSymbol::getFirstNode()
{
    if (firstNode == nullptr) firstNode = cfg.createNode(prefix + "0", true, false, this);
    return firstNode;
}

void FunctionSymbol::setFirstNode(CFGNode* firstNode)
{
    FunctionSymbol::firstNode = firstNode;
}

CFGNode* FunctionSymbol::getCurrentNode() const
{
    return currentNode;
}

const set<string>& FunctionSymbol::getVars()
{
    return vars;
}

void FunctionSymbol::addVar(const string& s)
{
    vars.insert(s);
}

bool FunctionSymbol::checkTypes(std::vector<VariableType>& potential)
{
    return ((potential.size() == paramTypes.size()) && paramTypes == potential);
}

const string FunctionSymbol::newStateName()
{
    return prefix + to_string(currentStates++);
}

const string& FunctionSymbol::getPrefix() const
{
    return prefix;
}

const string& FunctionSymbol::getIdent() const
{
    return ident;
}

bool FunctionSymbol::isOfType(VariableType c)
{
    return (c == returnType);
}

VariableType FunctionSymbol::getReturnType() const
{
    return returnType;
}

//each node should only be returned to once
void FunctionSymbol::addReturnSuccessor(CFGNode* returningTo)
{
    if (!returnTo.insert(returningTo).second) throw "return successor already in";
    returningTo->addParent(getLastNode());
}

void FunctionSymbol::addReturnSuccessors(const set<CFGNode*>& newRetSuccessors)
{
    for (const auto& ret : newRetSuccessors) addReturnSuccessor(ret);
}

void FunctionSymbol::clearReturnSuccessors()
{
    string lastName = getLastNode()->getName();
    for (CFGNode* child : returnTo) child->removeParent(lastName);
    returnTo.clear();
}

void FunctionSymbol::setReturnSuccessors(set<CFGNode*>& newRet)
{
    clearReturnSuccessors();
    for (const auto& newSucc : newRet) addReturnSuccessor(newSucc);
}

const std::set<CFGNode*>& FunctionSymbol::getReturnSuccessors()
{
    return returnTo;
}

void FunctionSymbol::removeReturnSuccessor(const std::string& ret)
{
    auto it = find_if(returnTo.begin(), returnTo.end(), [&, ret](CFGNode* other){return other->getName() == ret;});
    if (it == returnTo.end()) throw "couldnt find ret successor";
    else returnTo.erase(it);
}

//generation
void FunctionSymbol::genNewState(std::string n)
{
    if (!endedState) throw "Unfinished state";
    currentNode = cfg.createNode(n, true, false, this);
    endedState = false;
}

void FunctionSymbol::genEndState()
{
    if (endedState) throw "No state to end";
    currentNode->setInstructions(currentInstrs);
    currentInstrs.clear();
    endedState = true;
}

void FunctionSymbol::genJump(std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<JumpCommand>(s, linenum));
}

void FunctionSymbol::genPrint(std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<PrintCommand>(s, linenum));
}

void FunctionSymbol::genConditionalJump(std::string state, std::string lh, Relations::Relop r, std::string rh, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<JumpOnComparisonCommand>(state, lh, rh, r, linenum));
}

void FunctionSymbol::genPop(std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<PopCommand>(s, linenum));
}

void FunctionSymbol::genReturn(int linenum)
{
    if (endedState) throw "No state to add to";
    if (lastNode == nullptr)
    {
        lastNode = cfg.createNode(prefix + "fin", false, true, this);
    }
    currentInstrs.push_back(make_unique<JumpCommand>(lastNode->getName(), linenum));
}


void FunctionSymbol::genPush(std::string s, int linenum, FunctionSymbol* calledFunction)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<PushCommand>(s, linenum, calledFunction));
}

void FunctionSymbol::genInput(std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<InputVarCommand>(s, linenum));
}

void FunctionSymbol::genExpr(std::string lh, std::string t1, Op o, std::string t2, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<EvaluateExprCommand>(lh, t1, o, t2, linenum));
}

void FunctionSymbol::genVariableDecl(VariableType t, std::string n, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<DeclareVarCommand>(t, n, linenum));

    //find wont work for whatever reason
    for (const string& s : vars) if (s == n) return;
    vars.insert(n);
}

void FunctionSymbol::addCommand(unique_ptr<AbstractCommand> ac)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(move(ac));
}

void FunctionSymbol::genAssignment(std::string LHS, std::string RHS, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<AssignVarCommand>(LHS, RHS, linenum));
}

void FunctionSymbol::addCommands(std::vector<std::unique_ptr<AbstractCommand>>& acs)
{
    if (endedState) throw "No state to add to";
    currentInstrs.reserve(currentInstrs.size() + acs.size());
    currentInstrs.insert(currentInstrs.end(), make_move_iterator(acs.begin()), make_move_iterator(acs.end()));
}



