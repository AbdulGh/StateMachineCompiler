#include <stack>
#include <algorithm>

#include "Functions.h"

using namespace std;

FunctionSymbol::FunctionSymbol(VariableType rt, vector<VariableType> types, string id, string p, ControlFlowGraph& c):
    returnType(rt), paramTypes(move(types)), ident(move(id)), prefix(move(p)),
    currentStateNum(1), endedState(false), cfg(c), lastNode{nullptr}
    {currentNode = cfg.createNode(prefix + "0", false, false, this); firstNode = currentNode;}

void FunctionSymbol::mergeInto(FunctionSymbol *to)
{
    if (to->getPrefix() == getPrefix()) return;

    if (calls.size() != 1) throw "should have only one call";
    CFGNode* returningNode = calls.begin()->returnTo;
    returningNode->removeFunctionCall(lastNode->getName(), this);
    
    unsigned int pushedVarsRemaining = paramTypes.size();
    if (pushedVarsRemaining > 0)
    {
        vector<unique_ptr<AbstractCommand>>& myInstrs = firstNode->getInstrs();
        vector<unique_ptr<AbstractCommand>>& theirInstrs = to->lastNode->getInstrs();

        if (myInstrs.size() < pushedVarsRemaining * 2
            || theirInstrs.size() < pushedVarsRemaining + 1) throw "malformed call";
        
        auto myIterator = myInstrs.begin();
        
        while (pushedVarsRemaining > 0)
        {
            unique_ptr<AbstractCommand>& theirLastInstr = theirInstrs.back();
            if ((*myIterator)->getType() != CommandType::DECLAREVAR) throw "malformed call";
            myIterator = myInstrs.erase(myIterator);
            if ((*myIterator)->getType() != CommandType::POP 
             || theirLastInstr->getType() != CommandType::PUSH) throw "malformed call";
            myIterator = myInstrs.erase(myIterator);
            theirInstrs.pop_back();
            --pushedVarsRemaining;
        }

        unique_ptr<AbstractCommand>& theirLastInstr = theirInstrs.back();
        if (theirLastInstr->getType() != CommandType::PUSH) throw "should push state before arguments";
        PushCommand* pc = static_cast<PushCommand*>(theirLastInstr.get());
        if (pc->pushType != PushCommand::PUSHSTATE) throw "should push state before arguments";
        unsigned int numVarsToErase = pc->pushedVars;
        theirInstrs.pop_back();

        if (numVarsToErase > 0)
        {
            vector<unique_ptr<AbstractCommand>>& returnToInstrs = returningNode->getInstrs();

            if (theirInstrs.size() < numVarsToErase || returnToInstrs.size() < numVarsToErase)
            {
                throw "should pop/push local variables";
            }

            auto returnToIterator = returnToInstrs.begin();
            while (numVarsToErase > 0)
            {
                //just assume that these are actually the correct pushes/pops
                returnToIterator = returnToInstrs.erase(returnToIterator);
                myInstrs.pop_back();
                --numVarsToErase;
            }
        }
    }

    for (auto& pair : cfg.getCurrentNodes())
    {
        if (pair.second->getParentFunction()->getPrefix() == getPrefix()) pair.second->setParentFunction(to);
    }

    calls.clear();
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
        for (auto& cp : calls) cp.returnTo->removeParent(lastNode);
    }
    lastNode = ln;
    lastNode->setLast();
    for (auto& cp : calls) cp.returnTo->addParent(lastNode);
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
    return c == returnType;
}

VariableType FunctionSymbol::getReturnType() const
{
    return returnType;
}

unsigned int FunctionSymbol::numCalls()
{
    return calls.size();
}

//each node should only be returned to once
void FunctionSymbol::addFunctionCall(CFGNode *calling, CFGNode *returnTo, unsigned int numPushedVars)
{
    if (!calls.insert(FunctionCall(calling, returnTo, numPushedVars)).second) throw "already know about this call";
    returnTo->addParent(getLastNode());
}

const std::set<FunctionCall>& FunctionSymbol::getFunctionCalls() const
{
    return calls;
}

void FunctionSymbol::replaceReturnState(CFGNode *going, CFGNode* replaceWith)
{
    auto callsIt = calls.begin();
    while (callsIt != calls.end())
    {
        if (callsIt->returnTo->getName() == going->getName())
        {
            //find actual function call
            vector<unique_ptr<AbstractCommand>>& instrs = callsIt->caller->getInstrs();

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
                ++instrIt;
            }
            if (!found) throw "could not find push in pushing state";

            addFunctionCall(callsIt->caller, replaceWith, callsIt->numPushedVars);
            calls.erase(callsIt);
            replaceWith->addFunctionCall(callsIt->caller, this);
            return;
        }
        ++callsIt;
    }
    throw "could not find function call";
}

//todo check if I need to erase the calls themselves
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

void FunctionSymbol::removeFunctionCall(const string& calling, const string& ret, bool fixCalling)
{
    // instructions used in a function call:
    // FIRST STATE:
    // first push the function vars used so far
    // push the return state
    // push parameters
    // jump to first state of called function

    // CALLED FUNCTION:
    // declare and pop parameters
    // do function
    // when it's time to return, set retX to value to return

    // RETURNED TO STATE:
    // first pop function local vars
    // if the function return value is being assigned to something, copy it over


    //find if we can find this specific call, and if this is the only time this node is returned to
    unsigned int numRet = 0;
    unsigned long numParams = paramTypes.size();
    auto callsIterator = calls.begin();
    CFGNode* foundLeavingNode = nullptr;
    while (callsIterator != calls.end())
    {
        auto pair = *callsIterator;
        bool iteratorIncremented = false;
        if (pair.returnTo->getName() == ret)
        {
            ++numRet;
            if (!foundLeavingNode && pair.caller->getName() == calling)
            {
                foundLeavingNode = pair.returnTo;
                foundLeavingNode->removeParent(lastNode);

                //if only one call returns to returnTo, erase its params
                if (foundLeavingNode->getNumPushingStates() < 2 && numParams > 0)
                {
                    vector<unique_ptr<AbstractCommand>>& returnToInstrs = foundLeavingNode->getInstrs();
                    if (returnToInstrs.size() < numParams) throw "should have popped local vars";
                    returnToInstrs.erase(returnToInstrs.begin(), returnToInstrs.begin() + numParams);
                }

                if (fixCalling)
                {
                    //remove the stuff pushed onto the stack
                    vector<unique_ptr<AbstractCommand>>& pushingInstrs =  pair.caller->getInstrs();
                    bool found = false;
                    unsigned int instrIndex = 0;
                    while (instrIndex < pushingInstrs.size())
                    {
                        AbstractCommand* ac = pushingInstrs[instrIndex].get();
                        if (ac->getType() == CommandType::PUSH && ac->getData() == ret)
                        {
                            auto pc = static_cast<PushCommand*>(ac);
                            if (pc->pushType == PushCommand::PUSHSTATE)
                            {
                                if (pc->calledFunction->getIdent() != ident) throw "should be me";

                                //firstly erase stuff in calling node
                                unsigned int beginEraseIndex = instrIndex - callsIterator->numPushedVars;
                                if (beginEraseIndex < 0) throw "should have pushed local vars beforehand";
                                unsigned int stopEraseIndex = instrIndex + numParams + 1;
                                if (instrIndex + numParams > pushingInstrs.size())
                                {
                                    throw "should have pushed function params afterhand";
                                }

                                pushingInstrs.erase(pushingInstrs.begin() + beginEraseIndex,
                                                    pushingInstrs.begin() + stopEraseIndex);

                                found = true;
                                break;
                            }
                        }
                        ++instrIndex;
                    }
                    if (!found) throw "couldnt find push in pushing state";
                }
                callsIterator = calls.erase(callsIterator);
                iteratorIncremented = true;
            }
        }
        if (!iteratorIncremented) ++callsIterator;
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



