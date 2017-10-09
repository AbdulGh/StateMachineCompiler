#include <stack>
#include <algorithm>

#include "Functions.h"

using namespace std;

FunctionSymbol::FunctionSymbol(VariableType rt, std::vector<VariableType> types, std::string in, ControlFlowGraph& c):
    returnType(rt), paramTypes(move(types)), prefix(move(in)),
    currentStates(1), endedState(false), cfg(c)
    {currentNode = cfg.createNode(prefix + "0", false, false, this);}


//assumes the entire function is reachable from the first node (most unreachable parts will be removed by symbolic execution)
void FunctionSymbol::giveNodesTo(FunctionSymbol* to)
{
    to->setLastNode(getLastNode());
    stack<shared_ptr<CFGNode>> toConvert({getFirstNode()});
    while (!toConvert.empty())
    {
        shared_ptr<CFGNode> converting = (toConvert.top()); //why will this only work with parentheses?
        toConvert.pop();
        converting->setParentFunction(to);
        if (converting->getCompSuccess() != nullptr
            && converting->getCompSuccess()->getParentFunction()->getPrefix() == getPrefix())
        {
            toConvert.push(converting->getCompSuccess());
        }
        if (converting->getCompFail() != nullptr
            && converting->getCompFail()->getParentFunction()->getPrefix() == getPrefix())
        {
            toConvert.push(converting->getCompFail());
        }
    }
}

const shared_ptr<CFGNode>& FunctionSymbol::getLastNode()
{
    if (lastNode == nullptr) lastNode = cfg.createNode(prefix + "fin", false, true, this);
    return lastNode;
}

void FunctionSymbol::setLastNode(const shared_ptr<CFGNode>& ln)
{
    if (lastNode != nullptr)
    {
        lastNode->setLast(false);
        for (const auto& ret : returnTo) ret->removeParent(lastNode);
    }
    lastNode = ln;
    lastNode->setLast();
    for (const auto& ret : returnTo) ret->addParent(lastNode);
}

const shared_ptr<CFGNode>& FunctionSymbol::getFirstNode()
{
    if (firstNode == nullptr) firstNode = cfg.createNode(prefix + "0", true, false, this);
    return firstNode;
}

void FunctionSymbol::setFirstNode(const shared_ptr<CFGNode>& firstNode)
{
    FunctionSymbol::firstNode = firstNode;
}

const shared_ptr<CFGNode>& FunctionSymbol::getCurrentNode() const
{
    return currentNode;
}


const vector<string>& FunctionSymbol::getVars()
{
    return vars;
}

void FunctionSymbol::addVar(string s)
{
    vars.push_back(s);
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

bool FunctionSymbol::isOfType(VariableType c)
{
    return (c == returnType);
}

VariableType FunctionSymbol::getReturnType() const
{
    return returnType;
}

void FunctionSymbol::addReturnSuccessor(CFGNode* returningTo)
{
    for (const auto& ptr : returnTo)
    {
        if (ptr->getName() == returningTo->getName()) return;
    }

    returnTo.insert(returningTo);
    returningTo->addParent(getLastNode()->shared_from_this());
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
    auto eraseIt = find_if(returnTo.begin(), returnTo.end(),
                                  [&, ret](CFGNode* node)
                                  { return node->getName() == ret; });
    CFGNode* debug = (*eraseIt);
    if (eraseIt != returnTo.end()) returnTo.erase(eraseIt);
    //returnTo.erase(eraseEnd, returnTo.end());
    /*for (auto it = returnTo.begin(); it != returnTo.end(); it++)
    {
        if ((*it)->getName() == ret)
        {
            (*it)->removeParent(lastNode);
            returnTo.erase(it);
            break;
        }
    }*/
}

/*generation*/
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


void FunctionSymbol::genPush(std::string s, int linenum, FunctionSymbol* cf)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<PushCommand>(s, linenum, cf));
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
    vars.push_back(n);
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



