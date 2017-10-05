//
// Created by abdul on 28/09/17.
//

#include <algorithm>
#include <map>
#include <set>
#include <stack>
#include <memory>

#include "../compile/Functions.h"
#include "CFG.h"

using namespace std;

CFGNode::CFGNode(ControlFlowGraph& p, FunctionSymbol& pf, string n, bool last):
        parentGraph(p), name(move(n)), isLast(last), comp{}, parentFunction(pf),
        compSuccess{}, compFail{}, jumpline(-1) {}


const string &CFGNode::getName() const
{
    return name;
}

void CFGNode::putSource(stringstream& outs, bool makeState, string delim)
{
    if (makeState)
    {
        outs << name << delim;
        for (shared_ptr<AbstractCommand>& ac: instrs) outs << ac->translation(delim);
        if (compSuccess != nullptr) outs << comp->translation(delim);
        if (compFail != nullptr) outs << JumpCommand(compFail->getName(), jumpline).translation(delim);
        else outs << ReturnCommand(jumpline).translation(delim);
        outs << "end" << delim;
    }
    else for (shared_ptr<AbstractCommand>& ac: instrs) outs << ac->translation(delim);
}

void CFGNode::putDotNode(stringstream& outs)
{
    outs << getName() << "[label='<B><I>" << getName() << "</I></B>\\n";
    putSource(outs, false, "\\n");
    outs << "'];\n";
    if (compSuccess != nullptr)
    {
        string trans = comp->translation("");
        outs << getName() << "->" << compSuccess->getName()
             << "[label='" << trans << "'];\n";
    }
    if (compFail != nullptr)
    {
        if (comp != nullptr)
        {
            string trans = comp->negatedTranslation("");
            outs << getName() << "->" << compFail->getName()
                 << "[label='" << trans << "'];\n";
        }
        else outs << getName() << "->" << compFail->getName()
                  << "[label='jump'];\n";
    }
}

bool CFGNode::constProp()
{
    unordered_map<string,string> assignments;
    stack<vector<shared_ptr<AbstractCommand>>::iterator> pushedThings;

    auto it = instrs.begin();
    vector<shared_ptr<AbstractCommand>> newInstrs;
    newInstrs.reserve(instrs.size()); //avoid reallocation to keep iterators in pushedThings valid

    while (it != instrs.end())
    {
        shared_ptr<AbstractCommand> current = move(*it);
        if (current->getType() == CommandType::CHANGEVAR)
        {
            assignments.erase(current->getData());
            newInstrs.push_back(current);
        }
        else if (current->getType() == CommandType::ASSIGNVAR)
        {
            shared_ptr<AssignVarCommand> avc = static_pointer_cast<AssignVarCommand>(current);
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
            if (avc->getData() != avc->RHS) newInstrs.push_back(current);
        }
        else if (current->getType() == CommandType::EXPR)
        {
            shared_ptr<EvaluateExprCommand> eec = static_pointer_cast<EvaluateExprCommand>(current);
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
                    newInstrs.push_back(make_shared<AssignVarCommand>(eec->getData(), resultstr, eec->getLineNum()));
                }
                catch (invalid_argument&)
                {
                    if (eec->op == PLUS)
                    {
                        string result = eec->term1 + eec->term2;
                        assignments[eec->getData()] = result;
                        newInstrs.push_back(make_shared<AssignVarCommand>(eec->getData(), result, eec->getLineNum()));
                    }
                    else throw runtime_error("Strings only support +");
                }
            }
            else
            {
                assignments.erase(eec->getData());
                newInstrs.push_back(current);
            }
        }
        else if (current->getType() == CommandType::PUSH)
        {
            shared_ptr<PushCommand> pushc = static_pointer_cast<PushCommand>(current);
            if (pushc->pushType == PushCommand::PUSHSTR)
            {
                auto pushedVarIt = assignments.find(current->getData());
                if (pushedVarIt != assignments.end()) current->setData(pushedVarIt->second);
            }
            newInstrs.push_back(current);
            pushedThings.push(prev(newInstrs.end()));
        }
        else if (current->getType() == CommandType::POP && !pushedThings.empty())
        {
            shared_ptr<PopCommand> popc = static_pointer_cast<PopCommand>(current);
            auto stackTop = pushedThings.top();
            shared_ptr<PushCommand> pushc = static_pointer_cast<PushCommand>(*stackTop);
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
                        newInstrs.push_back(make_shared<AssignVarCommand>
                                                    (current->getData(), pushc->getData(), current->getLineNum()));
                    }
                    newInstrs.erase(stackTop);
                }
            }
        }
        else newInstrs.push_back(current);
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
    else if (compFail == nullptr && !pushedThings.empty()) //there should be a state on top
    {
        if (!isLast) throw "returning but not last";
        auto stackTop = pushedThings.top();
        shared_ptr<PushCommand> pushc = static_pointer_cast<PushCommand>(*stackTop);
        if (pushc->pushType != PushCommand::PUSHSTATE) throw runtime_error("tried to jump to var");
        shared_ptr<CFGNode> jumpingTo = parentGraph.getNode(pushc->getData());
        jumpingTo->removePushingState(name);
        if (jumpingTo == nullptr) throw runtime_error("Tried to jump to nonexistent state");
        compFail = jumpingTo;
        newInstrs.erase(stackTop);
        vector<CFGNode*>& returnTo = parentFunction.getReturnTo();
        if (returnTo.size() != 1) throw "check";
        returnTo.clear();
        skippedReturn = true;
    }
    instrs = move(newInstrs);
    return skippedReturn;
}

bool CFGNode::swallowNode(shared_ptr<CFGNode> other)
{
    if (compSuccess == nullptr)
    {
        vector<CFGNode*>& returnTo = parentFunction.getReturnTo();
        if (compFail != nullptr && compFail->getName() == other->getName()
            || isLast && returnTo.size() == 1 && returnTo.at(0)->getName() == other->getName())
        {
            vector<shared_ptr<AbstractCommand>> newInstrs = instrs;
            vector<shared_ptr<AbstractCommand>>& addingInstrs = other->getInstrs();
            newInstrs.reserve(instrs.size() + addingInstrs.size() + 3);

            for (shared_ptr<AbstractCommand>& newInst : addingInstrs)
            {
                newInstrs.push_back(newInst->clone());
            }
            setInstructions(newInstrs);
            if (other->getComp() != nullptr)
            {
                setComp(static_pointer_cast<JumpOnComparisonCommand>(other->getComp()->clone()));
            }
            else
            {
                setComp(nullptr);
                if (other->getCompFail() == nullptr)
                {
                    if (!other->isLastNode() || other->getPredecessors().size() != 1) throw "uh-oh";
                    if (other->getParentFunction().getPrefix() != parentFunction.getPrefix())
                    {
                        //todo use giveNodes
                    }
                }
            }
            setCompSuccess(other->getCompSuccess());
            setCompFail(other->getCompFail());
            return true;
        }
    }
    else if (compFail != nullptr && compFail->getName() == other->getName()
             && compFail->getCompSuccess() == nullptr && compFail->getInstrs().empty())
    {
        setCompFail(other->getCompFail());
        return true;
    }
    return false;
}

void CFGNode::setInstructions(vector<shared_ptr<AbstractCommand>>& in)
{
    instrs.clear();
    auto it = in.begin();

    while (it != in.end()
           && (*it)->getType() != CommandType::JUMP
           && (*it)->getType() != CommandType::CONDJUMP)
    {
        shared_ptr<AbstractCommand> current = move(*it);
        instrs.push_back(current);
        if (current->getType() == CommandType::PUSH)
        {
            shared_ptr<PushCommand> pc = static_pointer_cast<PushCommand>(current);
            if (pc->pushType == PushCommand::PUSHSTATE)
            {
                shared_ptr<CFGNode> pushedNode = parentGraph.getNode(current->getData());
                if (pushedNode == nullptr)
                    parentGraph.createNode(pc->getData(), false, false)
                            ->addPushingState(shared_from_this()); //will be created properly later
                else pushedNode->addPushingState(shared_from_this(), true);
            }
        }
        ++it;
    }

    if (it == in.cend()) return;

    if ((*it)->getType() == CommandType::CONDJUMP)
    {
        compSuccess = parentGraph.getNode((*it)->getData());
        if (compSuccess == nullptr) compSuccess = parentGraph.createNode((*it)->getData(), false, false);
        compSuccess->addParent(shared_from_this());
        comp = static_pointer_cast<JumpOnComparisonCommand>(move(*it)->clone());

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

shared_ptr<JumpOnComparisonCommand> CFGNode::getComp()
{
    return comp;
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

vector<shared_ptr<AbstractCommand>>& CFGNode::getInstrs()
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

void CFGNode::setComp(const shared_ptr<JumpOnComparisonCommand>& comparison)
{
    comp = comparison;
}

void CFGNode::setLast(bool last)
{
    isLast = last;
}

ControlFlowGraph& CFGNode::getParentGraph() const
{
    return parentGraph;
}

FunctionSymbol& CFGNode::getParentFunction() const
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

void CFGNode::addPushingState(const shared_ptr<CFGNode>& cfgn, bool idempotent)
{
    for (const auto& ptr : pushingStates)
    {
        if (ptr->getName() == cfgn->getName()) return;
    }
    pushingStates.push_back(cfgn);
    parentFunction.addReturnSuccessor(cfgn.get());
}

void CFGNode::removePushes()
{
    auto it = pushingStates.begin();
    while (it != pushingStates.end())
    {
        vector<shared_ptr<AbstractCommand>>& pushingInstrs =  (*it)->instrs;
        auto instructionIt = pushingInstrs.begin();
        bool found = false;
        while (instructionIt != pushingInstrs.end())
        {
            shared_ptr<AbstractCommand> ac = *instructionIt;
            if (ac->getType() == CommandType::PUSH)
            {
                shared_ptr<PushCommand> pc = static_pointer_cast<PushCommand>(ac);
                if (pc->pushType == PushCommand::PUSHSTATE && pc->getData() == name)
                {
                    pushingInstrs.erase(instructionIt);
                    found = true;
                }
            }
            ++instructionIt;
        }
        if (!found) throw "couldnt find push in pushing state";
        (*it)->parentFunction.removeReturnSuccessor(name);
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
        vector<shared_ptr<AbstractCommand>>& pushingInstrs =  (*it)->instrs;
        auto instructionIt = pushingInstrs.begin();
        bool found = false;
        while (instructionIt != pushingInstrs.end())
        {
            shared_ptr<AbstractCommand> ac = *instructionIt;
            if (ac->getType() == CommandType::PUSH)
            {
                if (ac->getData() == name)
                {
                    ac->setData(other);
                    found = true;
                    toReplaceWith->addPushingState(*it);
                }
            }
            ++instructionIt;
        }
        if (!found) throw "couldnt find push in pushing state";
        (*it)->parentFunction.removeReturnSuccessor(name);
        it = pushingStates.erase(it);
    }
}

void CFGNode::removePushingState(const string& bye)
{
    bool found = false;
    for (auto it = pushingStates.begin(); it != pushingStates.end(); it++)
    {
        if ((*it)->getName() == bye)
        {
            found = true;
            pushingStates.erase(it);
            break;
        }
    }
    if (!found) throw "couldnt find state in pushing states";
}

void CFGNode::prepareToDie()
{
    if (getCompFail() != nullptr) getCompFail()->removeParent(name);
    if (getCompSuccess() != nullptr) getCompSuccess()->removeParent(name);
    removePushes();

    //find out if we push states
    for (const auto& ac : instrs)
    {
        if (ac->getType() == CommandType::PUSH)
        {
            shared_ptr<PushCommand> pc = static_pointer_cast<PushCommand>(ac);
            if (pc->pushType == PushCommand::PUSHSTATE)
            {
                parentGraph.getNode(pc->getData())->removePushingState(name);
            }
        }
    }
}