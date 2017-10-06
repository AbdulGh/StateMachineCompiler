#include <stack>

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
    if (lastNode == nullptr)
    {
        vector<shared_ptr<AbstractCommand>> returnCommand({make_shared<ReturnCommand>(-1)});
        lastNode = cfg.createNode(prefix + "fin", false, true, this);
    }

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


vector<CFGNode*>& FunctionSymbol::getReturnTo()
{
    return returnTo;
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

    returnTo.push_back(returningTo);
    returningTo->addParent(getLastNode()->shared_from_this());

    printf("%s added as return succ of %s\n", returningTo->getName().c_str(), this->getPrefix().c_str());
}

void FunctionSymbol::addReturnSuccessors(const vector<CFGNode*>& newRetSuccessors)
{
    for (const auto& ret : newRetSuccessors) addReturnSuccessor(ret);
}

void FunctionSymbol::clearReturnSuccessors()
{
    string lastName = getLastNode()->getName();
    for (CFGNode* child : returnTo) child->removeParent(lastName);
    returnTo.clear();
}

void FunctionSymbol::setReturnSuccessors(vector<CFGNode*>& newRet)
{
    clearReturnSuccessors();
    for (const auto& newSucc : newRet) addReturnSuccessor(newSucc);

}

const std::vector<CFGNode*>& FunctionSymbol::getReturnSuccessors()
{
    return returnTo;
}

void FunctionSymbol::removeReturnSuccessor(const std::string& ret)
{
    printf("%s being removed as return succ of %s\n", ret.c_str(), this->getPrefix().c_str());
    bool found = false;
    for (auto it = returnTo.begin(); it != returnTo.end(); it++)
    {
        if ((*it)->getName() == ret)
        {
            returnTo.erase(it);
            found = true;
            break;
        }
    }
    if (!found) throw "couldnt find return successor";
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
    currentInstrs.push_back(make_shared<JumpCommand>(s, linenum));
}

void FunctionSymbol::genPrint(std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_shared<PrintCommand>(s, linenum));
}

void FunctionSymbol::genConditionalJump(std::string state, std::string lh, Relations::Relop r, std::string rh, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_shared<JumpOnComparisonCommand>(state, lh, rh, r, linenum));
}

void FunctionSymbol::genPop(std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_shared<PopCommand>(s, linenum));
}

void FunctionSymbol::genReturn(int linenum)
{
    if (endedState) throw "No state to add to";
    if (lastNode == nullptr)
    {
        vector<shared_ptr<AbstractCommand>> returnCommand({make_shared<ReturnCommand>(-1)});
        lastNode =
                cfg.createNode(prefix + "fin", false, true, this);
        lastNode->setInstructions(returnCommand);
    }
    currentInstrs.push_back(make_shared<JumpCommand>(lastNode->getName(), linenum));
}


void FunctionSymbol::genPush(PushCommand::PushType pt, std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_shared<PushCommand>(pt, s, linenum));
}

void FunctionSymbol::genInput(std::string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_shared<InputVarCommand>(s, linenum));
}

void FunctionSymbol::genExpr(std::string lh, std::string t1, Op o, std::string t2, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_shared<EvaluateExprCommand>(lh, t1, o, t2, linenum));
}

void FunctionSymbol::genVariableDecl(VariableType t, std::string n, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_shared<DeclareVarCommand>(t, n, linenum));

    //find wont work for whatever reason
    for (const string& s : vars) if (s == n) return;
    vars.push_back(n);
}

void FunctionSymbol::addCommand(shared_ptr<AbstractCommand> ac)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(ac);
}

void FunctionSymbol::genAssignment(std::string LHS, std::string RHS, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_shared<AssignVarCommand>(LHS, RHS, linenum));
}

void FunctionSymbol::addCommands(std::vector<std::shared_ptr<AbstractCommand>> acs)
{
    if (endedState) throw "No state to add to";
    currentInstrs.reserve(currentInstrs.size() + acs.size());
    currentInstrs.insert(currentInstrs.end(), acs.begin(), acs.end());
}



