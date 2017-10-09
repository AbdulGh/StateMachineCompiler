//
// Created by abdul on 28/09/17.
//

#include <algorithm>
#include <map>
#include <set>
#include <stack>
#include <memory>
#include <iostream>

#include "../compile/Functions.h"
#include "CFG.h"

using namespace std;

CFGNode::CFGNode(ControlFlowGraph& p, FunctionSymbol* pf, string n, bool last):
        parentGraph(p), name(move(n)), isLast(last), comp{}, parentFunction(pf),
        compSuccess{}, compFail{}, jumpline(-1) {}


const string &CFGNode::getName() const
{
    return name;
}

void CFGNode::printSource(bool makeState, std::string delim)
{
    if (makeState)
    {
        cout << name << delim;
        for (auto& ac: instrs)
        {
            if (ac == nullptr) cout << "null\n"; //debug
            else
            {
                string debug = ac->translation(delim);
                cout << debug;
            }
        }
        if (compSuccess != nullptr) cout << comp->translation(delim);
        if (compFail != nullptr) cout << JumpCommand(compFail->getName(), jumpline).translation(delim);
        else cout << ReturnCommand(jumpline).translation(delim);
        cout << "end" + delim;
    }
    else for (auto& ac: instrs) cout << ac->translation(delim);
    /*f (makeState)
    {
        cout << name << delim;
        for (shared_ptr<AbstractCommand>& ac: instrs)
        {
            if (ac == nullptr) cout << "null\n"; //debug
            else cout << ac->translation(delim);
        }
        if (compSuccess != nullptr) cout << comp->translation(delim);
        if (compFail != nullptr) cout << JumpCommand(compFail->getName(), jumpline).translation(delim);
        else cout << ReturnCommand(jumpline).translation(delim);
        cout << "end" << delim;
    }
    else for (shared_ptr<AbstractCommand>& ac: instrs) cout << ac->translation(delim);
    cout << "\n";*/
}

void CFGNode::printDotNode()
{
    cout << getName() << "[label='<B><I>" << getName() << "</I></B>\\n";
    printSource(false, "\\n");
    cout << "'];\n";
    if (compSuccess != nullptr)
    {
        string trans = comp->translation("");
        cout << getName() << "->" << compSuccess->getName()
             << "[label='" << trans << "'];\n";
    }
    if (compFail != nullptr)
    {
        if (comp != nullptr)
        {
            string trans = comp->negatedTranslation("");
            cout << getName() << "->" << compFail->getName()
                 << "[label='" << trans << "'];\n";
        }
        else cout << getName() << "->" << compFail->getName()
                  << "[label='jump'];\n";
    }
}

bool CFGNode::constProp()
{
    unordered_map<string,string> assignments;
    stack<vector<unique_ptr<AbstractCommand>>::iterator> pushedThings;

    auto it = instrs.begin();
    vector<unique_ptr<AbstractCommand>> newInstrs;
    newInstrs.reserve(instrs.size()); //avoid reallocation to keep iterators in pushedThings valid

    while (it != instrs.end())
    {
        unique_ptr<AbstractCommand> current = move(*it);
        if (current->getType() == CommandType::CHANGEVAR)
        {
            assignments.erase(current->getData());
            newInstrs.push_back(move(current));
        }
        else if (current->getType() == CommandType::ASSIGNVAR)
        {

            auto avc = static_cast<AssignVarCommand*>(current.get());
            if (AbstractCommand::getStringType(avc->RHS) != AbstractCommand::StringType::ID)
            {
                assignments[avc->getData()] = avc->RHS;
            }
            else
            {
                unordered_map<string, string>::const_iterator constit = assignments.find(avc->RHS);
                if (constit != assignments.end())
                {
                    assignments[avc->getData()] = constit->second;
                    avc->RHS = constit->second;
                }
                else assignments[avc->getData()] = avc->RHS;
            }
            if (avc->getData() != avc->RHS) newInstrs.push_back(move(current));
        }
        else if (current->getType() == CommandType::EXPR)
        {
            EvaluateExprCommand* eec = static_cast<EvaluateExprCommand*>(current.get());
            //literals will not be found
            if (AbstractCommand::getStringType(eec->term1) == AbstractCommand::StringType::ID)
            {
                unordered_map<string, string>::const_iterator t1it = assignments.find(eec->term1);
                if (t1it != assignments.end()) eec->term1 = t1it->second;
            }

            if (AbstractCommand::getStringType(eec->term2) == AbstractCommand::StringType::ID)
            {
                unordered_map<string, string>::const_iterator t2it = assignments.find(eec->term2);
                if (t2it != assignments.end()) eec->term2 = t2it->second;
            }

            if (AbstractCommand::getStringType(eec->term1) != AbstractCommand::StringType::ID
                && AbstractCommand::getStringType(eec->term2) != AbstractCommand::StringType::ID)
            {
                try //todo simplify statements where t1 = t2
                {
                    double lhs = stod(eec->term1);
                    double rhs = stod(eec->term2);
                    double result = evaluateOp(lhs, eec->op, rhs);
                    string resultstr = to_string(result);
                    assignments[eec->getData()] = resultstr;
                    newInstrs.push_back(make_unique<AssignVarCommand>(eec->getData(), resultstr, eec->getLineNum()));
                }
                catch (invalid_argument&)
                {
                    if (eec->op == PLUS)
                    {
                        string result = eec->term1 + eec->term2;
                        assignments[eec->getData()] = result;
                        newInstrs.push_back(make_unique<AssignVarCommand>(eec->getData(), result, eec->getLineNum()));
                    }
                    else throw runtime_error("Strings only support +");
                }
            }
            else
            {
                assignments.erase(eec->getData());
                newInstrs.push_back(move(current));
            }
        }
        else if (current->getType() == CommandType::PUSH)
        {
            auto pushc = static_cast<PushCommand*>(current.get());
            if (pushc->pushType == PushCommand::PUSHSTR)
            {
                auto pushedVarIt = assignments.find(current->getData());
                if (pushedVarIt != assignments.end()) current->setData(pushedVarIt->second);
            }
            newInstrs.push_back(move(current));
            pushedThings.push(prev(newInstrs.end()));
        }
        else if (current->getType() == CommandType::POP && !pushedThings.empty())
        {
            auto popc = static_cast<PopCommand*>(current.get());
            auto stackTop = pushedThings.top();
            auto pushc = static_cast<PushCommand*>((*stackTop).get());
            if (popc->isEmpty())
            {
                if (pushc->pushType == PushCommand::PUSHSTATE)
                {
                    shared_ptr<CFGNode> node = parentGraph.getNode(pushc->getData());
                    if (node == nullptr) throw "found a bad state";
                    //node->removeParent(getName()); todo debug this stuff
                    /*for (auto returnSuccIt = returnTo.cbegin(); returnSuccIt != returnTo.cend(); returnSuccIt++)
                    {
                        if ((*returnSuccIt)->getName() == node->getName())
                        {
                            returnTo.erase(returnSuccIt);
                            continue;
                        }
                    }
                    throw "could not find state in return successors";*/
                }
                newInstrs.erase(pushedThings.top());
                pushedThings.pop();
            }
            else
            {
                if (pushc->pushType == PushCommand::PUSHSTATE) throw runtime_error("tried to pop state into var");
                else
                {
                    pushedThings.pop();
                    if (current->getData() != pushc->getData())
                    {
                        newInstrs.push_back(make_unique<AssignVarCommand>
                                                    (current->getData(), pushc->getData(), current->getLineNum()));
                    }
                    newInstrs.erase(stackTop);
                }
            }
        }
        else newInstrs.push_back(move(current));
        ++it;
    }

    bool skippedReturn = false;
    if (comp != nullptr)
    {
        if (comp->term1Type == AbstractCommand::StringType::ID &&
            assignments.find(comp->term1) != assignments.end()) comp->setTerm1(assignments[comp->term1]);
        if (comp->term2Type == AbstractCommand::StringType::ID &&
            assignments.find(comp->term2) != assignments.end()) comp->setTerm2(assignments[comp->term2]);

        //check for const comparison
        if (comp->term1Type != AbstractCommand::StringType::ID
            && comp->term2Type != AbstractCommand::StringType::ID)
        {
            if (comp->term1Type != comp->term2Type)
            {
                parentGraph.reporter.error(Reporter::TYPE, "Tried to compare literals of different types", comp->getLineNum());
                return false;
            }

            else
            {
                string trans = comp->translation("");
                parentGraph.reporter.optimising(Reporter::USELESS_OP, 
                                                "Constant comparison: '" + trans + "'",
                                                comp->getLineNum());

                //replace conditionals with true/false
                bool isTrue;
                if (comp->term1Type == AbstractCommand::StringType::DOUBLELIT)
                {
                    double d1 = stod(comp->term1);
                    double d2 = stod(comp->term2);
                    isTrue = Relations::evaluateRelop<double>(d1, comp->op, d2);
                }
                else isTrue = Relations::evaluateRelop<string>(comp->term1, comp->op, comp->term2);

                if (isTrue)
                {
                    if (getCompFail() != nullptr) getCompFail()->removeParent(getName());
                    setCompFail(getCompSuccess());
                }
                setComp(nullptr);
                setCompSuccess(nullptr);
            }
        }
    }
    if (compFail == nullptr && !pushedThings.empty()) //there should be a state on top
    {
        /*auto stackTop = pushedThings.top();
        shared_ptr<PushCommand> pushc = static_pointer_cast<PushCommand>(*stackTop);
        if (pushc->pushType != PushCommand::PUSHSTATE) throw runtime_error("tried to jump to var");
        shared_ptr<CFGNode> jumpingTo = parentGraph.getNode(pushc->getData());
        jumpingTo->removePushingState(name);
        pushc->calledFunction->removeReturnSuccessor(pushc->getData());
        if (jumpingTo == nullptr) throw runtime_error("Tried to jump to nonexistent state");
        compFail = jumpingTo;
        newInstrs.erase(stackTop);
        const set<CFGNode*>& returnTo = parentFunction->getReturnSuccessors();
        if (returnTo.size() != 1) throw "check";
        parentFunction->clearReturnSuccessors();
        skippedReturn = true;*/
    }
    instrs = move(newInstrs);
    return skippedReturn;
}

bool CFGNode::swallowNode(shared_ptr<CFGNode> other)
{
    const set<CFGNode*>& returnTo = parentFunction->getReturnSuccessors();
    bool otherIsOnlyRetSuccessor = isLast && returnTo.size() == 1 && (*returnTo.cbegin())->getName() == other->getName();
    if (compSuccess == nullptr) //unconditional jump to the other
    {
        if (compFail != nullptr && compFail->getName() == other->getName() || otherIsOnlyRetSuccessor)
        {
            vector<unique_ptr<AbstractCommand>> newInstrs = move(instrs);
            vector<unique_ptr<AbstractCommand>>& addingInstrs = other->getInstrs();
            newInstrs.reserve(instrs.size() + addingInstrs.size() + 3);

            for (auto& newInst : addingInstrs)
            {
                newInstrs.push_back(newInst->clone());
                if (newInst->getType() == CommandType::PUSH)
                {
                    auto pc = static_cast<PushCommand*>(newInst.get());
                    if (pc->pushType == PushCommand::PUSHSTATE)
                    {
                        shared_ptr<CFGNode> node = parentGraph.getNode(pc->getData());
                        if (node == nullptr) throw "pushing nonexistant node";
                        node->addPushingState(shared_from_this());
                    }
                }
            }
            setInstructions(newInstrs);
            if (other->getComp() != nullptr)
            {
                JumpOnComparisonCommand* jocc = other->getComp();
                setComp(make_unique<JumpOnComparisonCommand>(jocc->getData(), jocc->term1, jocc->term2, jocc->op, jocc->getLineNum()));
            }
            else setComp(nullptr);
            setCompSuccess(other->getCompSuccess());
            setCompFail(other->getCompFail());
            if (otherIsOnlyRetSuccessor) parentFunction->removeReturnSuccessor(other->getName());
            return true;
        }
    }
    if (other->getInstrs().empty() && other->getCompSuccess() == nullptr)
    {
        if (compSuccess != nullptr &&  other->getName() == compSuccess->getName())
        {
            if (other->getCompFail() == nullptr)
            {
                if (compFail == nullptr) setCompSuccess(nullptr); //always return anyway
                else //swap condition
                {
                    comp->op = Relations::negateRelop(comp->op);
                    compSuccess = compFail;
                    compFail = nullptr;
                }
            }
            else setCompSuccess(other->getCompFail());
        }

        else if (compFail != nullptr)
        {
            if (compFail->getName() != other->getName()) throw "should be compfail";
            setCompFail(other->getCompFail());
        }

        else if (otherIsOnlyRetSuccessor && predecessors.size() == 1) setCompFail(other->getCompFail());

        else return false;

        return true;
    }
    return false;
}

void CFGNode::setInstructions(vector<unique_ptr<AbstractCommand>>& in)
{
    instrs.clear();
    auto it = in.begin();

    while (it != in.end()
           && (*it)->getType() != CommandType::JUMP
           && (*it)->getType() != CommandType::CONDJUMP)
    {
        instrs.push_back(move(*it));
        ++it;
    }

    if (it == in.cend()) return;

    if ((*it)->getType() == CommandType::CONDJUMP)
    {
        JumpOnComparisonCommand* jocc = static_cast<JumpOnComparisonCommand*>(it->get());
        compSuccess = parentGraph.getNode(jocc->getData());
        if (compSuccess == nullptr) compSuccess = parentGraph.createNode(jocc->getData(), false, false);
        compSuccess->addParent(shared_from_this());
        setComp(make_unique<JumpOnComparisonCommand>(jocc->getData(), jocc->term1, jocc->term2, jocc->op, jocc->getLineNum()));

        if (++it == in.cend()) return;
    }

    if ((*it)->getType() == CommandType::JUMP)
    {
        string jumpto = (*it)->getData();
        jumpline = (*it)->getLineNum();
        if (jumpto == "return") compFail = nullptr;
        else
        {
            compFail = parentGraph.getNode(jumpto);
            if (compFail == nullptr) compFail = parentGraph.createNode(jumpto, false, false);
            compFail->addParent(shared_from_this());
        }
        if (++it != in.cend()) throw "Should end here";
    }
    else throw "Can only end w/ <=2 jumps";
}

bool CFGNode::addParent(shared_ptr<CFGNode> parent)
{
    auto it = predecessors.find(parent->getName());
    if (it != predecessors.end()) return false;
    predecessors[parent->getName()] = parent;
    return true;
}

void CFGNode::removeParent(shared_ptr<CFGNode> leaving)
{
    removeParent(leaving->getName());
}

void CFGNode::removeParent(const string& s)
{
    if (predecessors.erase(s) == 0) throw runtime_error("Parent '" + s + "' not found in '" + getName() + "'");
}

JumpOnComparisonCommand* CFGNode::getComp()
{
    return comp.get();
}

unordered_map<string, shared_ptr<CFGNode>>& CFGNode::getPredecessors()
{
    return predecessors;
}

shared_ptr<CFGNode> CFGNode::getCompSuccess()
{
    return compSuccess;
}

shared_ptr<CFGNode> CFGNode::getCompFail()
{
    return compFail;
}

vector<unique_ptr<AbstractCommand>>& CFGNode::getInstrs()
{
    return instrs;
}

void CFGNode::setCompSuccess(const shared_ptr<CFGNode>& compsucc)
{
    compSuccess = compsucc;
    if (compSuccess != nullptr)
    {
        if (comp == nullptr) throw "comp should be set";
        compSuccess->addParent(shared_from_this());
        comp->setData(compSuccess->getName());
    }
    else comp = nullptr;
}

void CFGNode::setCompFail(const shared_ptr<CFGNode>& compareFail)
{
    compFail = compareFail;
    if (compFail != nullptr) compFail->addParent(shared_from_this());
}

void CFGNode::setComp(unique_ptr<JumpOnComparisonCommand> comparison)
{
    if (comp != nullptr) comp.release();
    comp.swap(comparison);
}

void CFGNode::setLast(bool last)
{
    isLast = last;
    if (isLast && (parentFunction->getLastNode() == nullptr || parentFunction->getLastNode()->getName() != name))
    {
        throw "must be function last";
    }
}

void CFGNode::setParentFunction(FunctionSymbol *pf)
{
    parentFunction = pf;
}

ControlFlowGraph& CFGNode::getParentGraph() const
{
    return parentGraph;
}

FunctionSymbol* CFGNode::getParentFunction() const
{
    return parentFunction;
}

int CFGNode::getJumpline() const
{
    return jumpline;
}

bool CFGNode::isLastNode() const
{
    return isLast;
}

bool CFGNode::isFirstNode() const
{
    return name == parentFunction->getFirstNode()->getName();
}

void CFGNode::addPushingState(const shared_ptr<CFGNode>& cfgn)
{
    for (const auto& ptr : pushingStates) if (ptr->getName() == cfgn->getName()) return;
    pushingStates.push_back(cfgn);
}

void CFGNode::removePushes()
{
    auto it = pushingStates.begin();
    while (it != pushingStates.end())
    {
        shared_ptr<CFGNode> pushing = *it;
        vector<unique_ptr<AbstractCommand>>& pushingInstrs =  pushing->instrs;
        auto instructionIt = pushingInstrs.begin();
        bool found = false;
        while (instructionIt != pushingInstrs.end())
        {
            AbstractCommand* ac = (*instructionIt).get();
            if (ac->getType() == CommandType::PUSH && ac->getData() == name)
            {
                auto pc = static_cast<PushCommand*>(ac);
                if (pc->pushType == PushCommand::PUSHSTATE)
                {
                    pc->calledFunction->removeReturnSuccessor(name);
                    pushingInstrs.erase(instructionIt);
                    found = true;
                    break;
                }
            }
            ++instructionIt;
        }
        if (!found) throw "couldnt find push in pushing state";
        it = pushingStates.erase(it);
    }
}

void CFGNode::replacePushes(const std::string& other)
{
    shared_ptr<CFGNode> toReplaceWith = parentGraph.getNode(other);
    if (toReplaceWith == nullptr) throw "asked to replace w/ nonexistent node";

    auto it = pushingStates.begin();
    while (it != pushingStates.end())
    {
        shared_ptr<CFGNode> pushing = *it;
        vector<unique_ptr<AbstractCommand>>& pushingInstrs =  pushing->instrs;
        auto instructionIt = pushingInstrs.begin();
        bool found = false;
        while (instructionIt != pushingInstrs.end())
        {
            AbstractCommand* ac = (*instructionIt).get();
            if (ac->getType() == CommandType::PUSH && ac->getData() == name)
            {
                PushCommand* pc = static_cast<PushCommand*>(ac);
                if (pc->pushType == PushCommand::PUSHSTATE)
                {
                    pc->setData(other);
                    pc->calledFunction->removeReturnSuccessor(name);
                    pc->calledFunction->addReturnSuccessor(toReplaceWith.get());
                    found = true;
                    toReplaceWith->addPushingState(pushing);
                    break;
                }
            }
            ++instructionIt;
        }
        if (!found) throw "couldnt find push in pushing state";
        it = pushingStates.erase(it);
    }
}

void CFGNode::removePushingState(const string& bye)
{
    for (auto it = pushingStates.begin(); it != pushingStates.end();)
    {
        if ((*it)->getName() == bye) pushingStates.erase(it);
        else ++it;
    }
}

void CFGNode::prepareToDie()
{
    if (isLastNode()) throw "cant delete last node";
    if (getCompFail() != nullptr) getCompFail()->removeParent(name);
    if (getCompSuccess() != nullptr) getCompSuccess()->removeParent(name);
    removePushes();

    //find out if we push states
    for (const auto& ac : instrs)
    {
        if (ac->getType() == CommandType::PUSH)
        {
            auto pc = static_cast<PushCommand*>(ac.get());
            if (pc->pushType == PushCommand::PUSHSTATE)
            {
                parentGraph.getNode(pc->getData())->removePushingState(name);
                pc->calledFunction->removeReturnSuccessor(pc->getData());
            }
        }
    }
}