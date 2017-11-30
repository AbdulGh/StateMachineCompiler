#include <stack>
#include <algorithm>

#include "Functions.h"

using namespace std;

typedef pair<CFGNode*, CFGNode*> CallPair;

FunctionSymbol::FunctionSymbol(VariableType rt, vector<VariableType> types, string id, string p, ControlFlowGraph& c):
    returnType(rt), paramTypes(move(types)), ident(move(id)), prefix(move(p)),
    currentStateNum(1), endedState(false), cfg(c), lastNode{nullptr}
    {currentNode = cfg.createNode(prefix + "0", false, false, this); firstNode = currentNode;}

void FunctionSymbol::giveNodesTo(FunctionSymbol* to)
{
    if (to->getPrefix() == getPrefix()) return;

    if (lastNode != nullptr)
    {
        for (auto cp : calls)
        {
            cp.second->removeParent(lastNode);
            to->addFunctionCall(cp.first, cp.second);
        }

        lastNode->setLast(false);
    }
    else if (!calls.empty()) throw "should be empty";

    for (auto& pair : cfg.getCurrentNodes())
    {
        if (pair.second->getParentFunction()->getPrefix() == getPrefix()) pair.second->setParentFunction(to);
    }

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
        if (ln->getName() == lastNode->getName()) return;
        if (lastNode->getName() == lastNode->getParentGraph().getLast()->getName())
        {
            lastNode->getParentGraph().setLast(ln->getName());
        }
        lastNode->setLast(false);
        for (auto& cp : calls) cp.second->removeParent(lastNode);
    }
    lastNode = ln;
    lastNode->setLast();
    for (auto& cp : calls) cp.second->addParent(lastNode);
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

unsigned int FunctionSymbol::numParams()
{
    return paramTypes.size();
}

bool FunctionSymbol::checkTypes(vector<VariableType>& potential)
{
    return ((potential.size() == paramTypes.size()) && paramTypes == potential);
}

const string FunctionSymbol::newStateName()
{
    return prefix + to_string(currentStateNum++);
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
//todo incorporate num parameters
void FunctionSymbol::addFunctionCall(CFGNode *calling, CFGNode *returnTo)
{
    if (!calls.insert(CallPair(calling, returnTo)).second) throw "already know about this call";
    returnTo->addParent(getLastNode());
}

void FunctionSymbol::replaceReturnState(CFGNode *going, CFGNode* replaceWith)
{
    auto it = calls.begin();
    while (it != calls.end())
    {
        if (it->returnTo->getName() == going->getName())
        {
            //find actual function call
            vector<unique_ptr<AbstractCommand>>& instrs = it->caller->getInstrs();

            bool found = false;
            auto instrIt = instrs.begin();
            while (instrIt != instrs.end())
            {
                AbstractCommand* ac = (*instrIt).get();
                if (ac->getType() == CommandType::PUSH && ac->getData() == going->getName());
                {
                    PushCommand* pc = static_cast<PushCommand*>(ac);
                    if (pc->pushType == PushCommand::PUSHSTATE)
                    {
                        found = true;
                        pc->setData(replaceWith->getName());
                        break;
                    }
                }
                ++it;
            }
            if (!found) throw "could not find push in pushing state";

            addFunctionCall(it->caller, replaceWith);
            calls.erase(it);
            replaceWith->addFunctionCall(it->caller, this);
            return;
        }
        ++it;
    }
    throw "could not find function call";
}

void FunctionSymbol::clearFunctionCalls()
{
    string lastName = getLastNode()->getName();
    for (FunctionCall cp : calls) cp.returnTo->removeParent(lastName);
    calls.clear();
}

vector<CFGNode*> FunctionSymbol::getNodesReturnedTo()
{
    vector<CFGNode*> toReturn;
    for (const FunctionCall& call : calls) toReturn.push_back(call.returnTo);
    return toReturn;
}

FunctionCall FunctionSymbol::getOnlyFunctionCall()
{
    if (calls.size() != 1) throw "should have exactly one call";
    return *calls.begin();
}

void FunctionSymbol::removeFunctionCall(const string& calling, const string& ret)
{
    //find if we can find this specific call, and if this is the only time this node is returned to
    unsigned int numRet = 0;
    auto it = calls.begin();
    CFGNode* foundLeavingNode = nullptr;
    while (it != calls.end())
    {
        auto pair = *it;
        if (pair.returnTo->getName() == ret)
        {
            if (!foundLeavingNode && pair.caller->getName() == calling)
            {
                foundLeavingNode = pair.returnTo;

                //remove the stuff pushed onto the stack
                vector<unique_ptr<AbstractCommand>>& pushingInstrs =  pair.caller->getInstrs();
                auto instructionIt = pushingInstrs.begin();
                bool found = false;
                while (instructionIt != pushingInstrs.end())
                {
                    AbstractCommand* ac = (*instructionIt).get();
                    if (ac->getType() == CommandType::PUSH && ac->getData() == ret)
                    {
                        auto pc = static_cast<PushCommand*>(ac);
                        if (pc->pushType == PushCommand::PUSHSTATE)
                        {
                            if (pc->calledFunction->getIdent() != ident) throw "should be me";
                            unsigned long numParams = paramTypes.size();
                            instructionIt = pushingInstrs.erase(instructionIt);

                            while (numParams > 0)
                            {
                                if (instructionIt == pushingInstrs.end()) throw "expected more pushed arguments";
                                AbstractCommand* ac = (*instructionIt).get();
                                if (ac->getType() != CommandType::PUSH) throw "expected more push commands";
                                instructionIt = pushingInstrs.erase(instructionIt);
                                --numParams;
                            }
                            found = true;
                            break;
                        }
                    }
                    ++instructionIt;
                }
                if (!found) throw "couldnt find push in pushing state";
                it = calls.erase(it);
            }
            else ++it;
            ++numRet;
        }
        else ++it;
    }
    if (!foundLeavingNode) throw "couldnt find function call";
    if (numRet == 1) foundLeavingNode->removeParent(lastNode);
}

//generation
void FunctionSymbol::genNewState(string n)
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

void FunctionSymbol::genJump(string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<JumpCommand>(s, linenum));
}

void FunctionSymbol::genPrint(string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<PrintCommand>(s, linenum));
}

void FunctionSymbol::genConditionalJump(string state, string lh, Relations::Relop r, string rh, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<JumpOnComparisonCommand>(state, lh, rh, r, linenum));
}

void FunctionSymbol::genPop(string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<PopCommand>(s, linenum));
}

void FunctionSymbol::genReturn(int linenum)
{
    if (endedState) throw "No state to add to";
    if (lastNode == nullptr) lastNode = cfg.createNode(prefix + "fin", false, true, this);
    currentInstrs.push_back(make_unique<JumpCommand>(lastNode->getName(), linenum));
}


void FunctionSymbol::genPush(string s, int linenum, FunctionSymbol* calledFunction)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<PushCommand>(s, linenum, calledFunction));
}

void FunctionSymbol::genInput(string s, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<InputVarCommand>(s, linenum));
}

void FunctionSymbol::genExpr(string lh, string t1, Op o, string t2, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<EvaluateExprCommand>(lh, t1, o, t2, linenum));
}

void FunctionSymbol::genVariableDecl(VariableType t, string n, int linenum)
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

void FunctionSymbol::genAssignment(string LHS, string RHS, int linenum)
{
    if (endedState) throw "No state to add to";
    currentInstrs.push_back(make_unique<AssignVarCommand>(LHS, RHS, linenum));
}

void FunctionSymbol::addCommands(vector<unique_ptr<AbstractCommand>>& acs)
{
    if (endedState) throw "No state to add to";
    currentInstrs.reserve(currentInstrs.size() + acs.size());
    currentInstrs.insert(currentInstrs.end(), make_move_iterator(acs.begin()), make_move_iterator(acs.end()));
}



