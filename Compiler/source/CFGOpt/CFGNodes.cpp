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
#include "../symbolic/VarWrappers.h"
#include "../Command.h"
#include "CFG.h"

using namespace std;

CFGNode::CFGNode(ControlFlowGraph& p, FunctionSymbol* pf, string n, bool last):
        parentGraph(p), name(move(n)), isLast(last), comp{}, parentFunction(pf),
        compSuccess{}, compFail{}, jumpline(-1) {}


const string &CFGNode::getName() const
{
    return name;
}

string CFGNode::getSource(bool makeState, std::string delim, bool escape) const
{
    stringstream outs;

    if (makeState)
    {
        outs << name << delim;
        for (auto& ac: instrs)
        {
            outs << ac->translation(delim);
        }
        if (compSuccess != nullptr) outs << comp->translation(delim);
        if (compFail != nullptr) outs << JumpCommand(compFail->getName(), jumpline).translation(delim);
        else outs << ReturnCommand(jumpline).translation(delim);
        outs << "end" + delim;
    }
    else for (auto& ac: instrs) outs << ac->translation(delim);
    if (escape)
    {
        string out;
        for (char c : outs.str())
        {
            if (c == '"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else out += c;
        }
        return out;
    }
    return outs.str();
}

string CFGNode::getDotNode()
{
    stringstream outs;
    outs << getName() << "[label=<<B>" << getName() << "</B><br/>";
    outs << getSource(false, "<br/>", false);
    outs << "> shape=box];";
    return outs.str();
}

string CFGNode::getDotEdges()
{
    stringstream outs;
    if (compSuccess != nullptr)
    {
        string trans = comp->condition("");
        outs << getName() << "->" << compSuccess->getName()
             << "[label=\"" << trans << "\"];\n";
    }
    if (compFail != nullptr)
    {
        if (comp != nullptr)
        {
            string trans = comp->negatedCondition("");
            outs << getName() << "->" << compFail->getName()
                 << "[label=\" " << trans << "\"];\n";
        } else
            outs << getName() << "->" << compFail->getName()
                 << "[label=\"  jump\"];\n";
    }
    else
    {
        for (CFGNode* nodePointer : parentFunction->getNodesReturnedTo())
        {
            outs << name << "->" << nodePointer->getName()
                 << "[label=\"return\"];\n";
        }
    }

    return outs.str();
}

bool CFGNode::constProp(unordered_map<string,Atom> assignments)
{
    stack<vector<unique_ptr<AbstractCommand>>::iterator> pushedThings;

    auto it = instrs.begin();
    vector<unique_ptr<AbstractCommand>> newInstrs;
    newInstrs.reserve(instrs.size()); //avoid reallocation to keep iterators in pushedThings valid

    while (it != instrs.end())
    {
        unique_ptr<AbstractCommand> current = move(*it);
        switch (current->getType())
        {
            case CommandType::NONDET:
            {
                auto ndc = static_cast<NondetCommand *>(current.get());
                if (ndc->holding) assignments.erase(current->getVarWrapper()->getBaseName());
                newInstrs.push_back(move(current));
                break;
            }
            case CommandType::INPUTVAR:
            {
                assignments.erase(current->getVarWrapper()->getFullName());
                newInstrs.push_back(move(current));
                break;
            }
            case CommandType::ASSIGNVAR:
            {
                auto avc = static_cast<AssignVarCommand*>(current.get());
                const string& lhsname = avc->getVarWrapper()->getFullName();
                if (!avc->getAtom().isHolding()) assignments.emplace(lhsname, Atom(avc->getAtom()));
                else
                {
                    const string& vname = avc->getAtom().getVarWrapper()->getFullName();
                    unordered_map<string, Atom>::const_iterator constit = assignments.find(vname);
                    if (constit != assignments.end())
                    {
                        Atom found = constit->second;
                        if (found.isHolding()) break; //todo
                        assignments.emplace(lhsname, Atom(found));
                        avc->getAtom().become(found);
                    }
                    else assignments.emplace(lhsname, Atom(avc->getAtom()));
                }

                if (lhsname != string(avc->getAtom())) newInstrs.push_back(move(current));
                break;
            }
            case CommandType::EXPR:
            {
                EvaluateExprCommand* eec = static_cast<EvaluateExprCommand*>(current.get());
                const string& intoVar = eec->getVarWrapper()->getFullName();

                //literals will not be found
                if (eec->term1.isHolding())
                {
                    unordered_map<string, Atom>::const_iterator t1it = assignments.find(eec->term1.getVarWrapper()->getFullName());
                    if (t1it != assignments.end()) eec->term1 = Atom(t1it->second);

                }
    
                if (eec->term2.isHolding())
                {
                    unordered_map<string, Atom>::const_iterator t2it = assignments.find(eec->term2.getVarWrapper()->getFullName());
                    if (t2it != assignments.end()) eec->term2 = Atom(t2it->second);
                }
    
                if (eec->term1 == eec->term2 && eec->term1.isHolding())
                {
                    switch (eec->op)
                    {
                        case MINUS:
                        {
                            assignments.emplace(intoVar, Atom(0));
                            newInstrs.push_back(make_unique<AssignVarCommand>(eec->getVarWrapper()->clone(),
                                                                              Atom(0), eec->getLineNum()));
                            break;
                        }
                        case PLUS:
                        {
                            assignments.erase(intoVar);
                            eec->op = MULT;
                            eec->term2.set(2);
                            newInstrs.push_back(move(current));
                            break;
                        }
                        default:
                        {
                            assignments.erase(intoVar);
                            newInstrs.push_back(move(current));
                        }
                    }
                }//when we leave this at least one term is an ID so we dont enter the next condition
    
                if (!eec->term1.isHolding() && !eec->term2.isHolding())
                {
                    double lhs = eec->term1.getLiteral();
                    double rhs = eec->term2.getLiteral();
                    double result = evaluateOp(lhs, eec->op, rhs);
                    assignments.emplace(intoVar, Atom(result));
                    newInstrs.push_back(make_unique<AssignVarCommand>(eec->getVarWrapper()->clone(),
                                                                      Atom(result), eec->getLineNum()));
                }
                else
                {
                    assignments.erase(intoVar);
                    newInstrs.push_back(move(current));
                }
                break;
            }
            case CommandType::PRINT:
            {
                unordered_map<string, Atom>::const_iterator t1it = assignments.find(string(current->getAtom()));
                if (t1it != assignments.end()) current->getAtom().become(t1it->second);
                newInstrs.push_back(move(current));
                break;
            }
            case CommandType::PUSH:
            {
                auto pushc = static_cast<PushCommand*>(current.get());
                if (!pushc->pushesState())
                {
                    if (pushc->getAtom().isHolding())
                    {
                        auto pushedVarIt = assignments.find(pushc->getAtom().getVarWrapper()->getFullName());
                        if (pushedVarIt != assignments.end()) current->setAtom(pushedVarIt->second);
                    }
                }
                newInstrs.push_back(move(current));
                pushedThings.push(prev(newInstrs.end()));
                break;
            }
            case CommandType::POP:
            {
                if (!pushedThings.empty())
                {
                    auto popc = static_cast<PopCommand*>(current.get());
                    auto stackTop = pushedThings.top();
                    auto pushc = static_cast<PushCommand*>((*stackTop).get());
                    if (popc->isEmpty())
                    {
                        if (pushc->pushesState())
                        {
                            const std::string& nodename = pushc->getString();
                            CFGNode* node = parentGraph.getNode(nodename);
                            if (node == nullptr) throw std::runtime_error("found a bad state");
                            pushc->calledFunction->removeFunctionCall(name, nodename);
                            node->removeFunctionCall(name, pushc->calledFunction);
                        }
                        newInstrs.erase(pushedThings.top());
                        pushedThings.pop();
                    }
                    else
                    {
                        if (pushc->pushesState()) throw runtime_error("tried to pop state into var");
                        else
                        {
                            pushedThings.pop();
                            const string& popInto = current->getVarWrapper()->getFullName();
                            if (popInto != string(pushc->getAtom()))
                            {
                                newInstrs.push_back(make_unique<AssignVarCommand>
                                                            (current->getVarWrapper()->clone(),
                                                             pushc->getAtom(), current->getLineNum()));
                            }
                            newInstrs.erase(stackTop);
                        }
                    }
                    break;
                }
            }
            default:
                newInstrs.push_back(move(current));
        }
        ++it;
    }

    bool skippedReturn = false;

    if (comp != nullptr)
    {
        if (comp->term1.isHolding())
        {
            auto it = assignments.find(comp->term1.getVarWrapper()->getFullName());
            if (it != assignments.end()) comp->term1.become(it->second);
        }
        if (comp->term2.getType() == StringType::ID)
        {
            auto it = assignments.find(comp->term2.getVarWrapper()->getFullName());
            if (it != assignments.end()) comp->term2.become(it->second);
        }

        //check for const comparison
        if (comp->term1.getType() != StringType::ID
            && comp->term2.getType() != StringType::ID)
        {
            if (comp->term1.getType() != comp->term2.getType())
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
                if (Relations::evaluateRelop<double>(comp->term1.getLiteral(), comp->op, comp->term2.getLiteral()))
                {
                    if (getCompFail() != nullptr) getCompFail()->removeParent(getName());
                    else throw std::runtime_error("shouldnt happen at the end of a function call");
                    setCompFail(getCompSuccess());
                }
                else getCompSuccess()->removeParent(this);
                setComp(nullptr);
                setCompSuccess(nullptr);
            }
        }
    }

    instrs = move(newInstrs);
    return skippedReturn;
}

bool CFGNode::swallowNode(CFGNode* other)
{
    if (other->getName() == name) throw std::runtime_error("cant swallow self");

    const set<unique_ptr<FunctionCall>>& returnTo = parentFunction->getFunctionCalls();

    bool needlessFunctionCall = other->isFirstNode() && other->getPredecessorMap().size() == 1;

    bool needlessFunctionReturn = isLast && returnTo.size() == 1
                                && (*returnTo.cbegin())->returnTo->getName() == other->getName()
                                && other->getParentFunction()->numCalls() <= 1;

    if (compSuccess == nullptr)
    {
        if (needlessFunctionCall) other->parentFunction->mergeInto(parentFunction);
        else if (needlessFunctionReturn) return false; //will be taken care of on the other side


        if (compFail != nullptr && compFail->getName() == other->getName())
        {
            vector<unique_ptr<AbstractCommand>> newInstrs = move(instrs);
            vector<unique_ptr<AbstractCommand>>& addingInstrs = other->getInstrs();
            newInstrs.reserve(instrs.size() + addingInstrs.size() + 3);

            for (auto& newInst : addingInstrs)
            {
                newInstrs.push_back(newInst->clone());

                if (newInst->getType() == CommandType::PUSH)
                {
                    PushCommand* pc = static_cast<PushCommand*>(newInst.get());
                    if (pc->pushesState())
                    {
                        CFGNode* node = parentGraph.getNode(pc->getString());
                        if (node == nullptr) throw std::runtime_error("pushing nonexistent node");
                        else if (calledFunctionSymbol && !needlessFunctionCall) throw std::runtime_error("can only call one function");
                        if (!other->calledFunctionSymbol) throw std::runtime_error("other is pushing states w/out calling");
                        calledFunctionSymbol = other->calledFunctionSymbol;
                        node->addFunctionCall(this, pc->calledFunction);
                        pc->calledFunction->addFunctionCall(this, node, pc->pushedVars);
                    }
                }
            }
            
            setInstructions(newInstrs);
            
            if (other->getComp() != nullptr)
            {
                JumpOnComparisonCommand* jocc = other->getComp();
                setComp(make_unique<JumpOnComparisonCommand>(*jocc));
            }
            else setComp(nullptr);
            
            setCompSuccess(other->getCompSuccess());
            setCompFail(other->getCompFail());
            if (compSuccess != nullptr && compFail != nullptr && compSuccess->getName() == compFail->getName())
            {
                setComp(nullptr);
                setCompSuccess(nullptr);
            }

            return true;
        }
    }
    else if (other->getInstrs().empty() && other->getCompSuccess() == nullptr) //empty node that just jumps
    {
        if (compSuccess != nullptr && other->getName() == compSuccess->getName())
        {
            if (other->getCompFail() == nullptr)
            {
                if (compFail == nullptr)//always return anyway
                {
                    setCompSuccess(nullptr);
                    setComp(nullptr);
                }
                else //swap condition
                {
                    comp->op = Relations::negateRelop(comp->op);
                    comp->setString(compFail->getName());
                    compSuccess = compFail;
                    compFail = nullptr;
                }
            }
            else setCompSuccess(other->getCompFail());
        }

        else if (compFail != nullptr)
        {
            if (compFail->getName() != other->getName())  throw std::runtime_error("should be compfail");
            setCompFail(other->getCompFail());
        }

        else if (needlessFunctionReturn && predecessors.size() == 1) setCompFail(other->getCompFail());
        else return false;

        if (needlessFunctionReturn) parentFunction->clearFunctionCalls();
        if (compSuccess != nullptr && compFail != nullptr && compSuccess->getName() == compFail->getName())
        {
            setComp(nullptr);
            setCompSuccess(nullptr);
        }

        return true;
    }
    return false;
}

void CFGNode::appendDeclatation(std::string varName)
{
    instrs.push_back(make_unique<DeclareVarCommand>(move(varName), -1));
}

void CFGNode::appendArrayDeclaration(std::string varName, unsigned int size)
{
    instrs.push_back(make_unique<DeclareArrayCommand>(move(varName), size, -1));
}


void CFGNode::setInstructions(vector<unique_ptr<AbstractCommand>>& in)
{
    instrs.clear();
    auto it = in.begin();

    stack<AbstractCommand*> intraNodeStack; //std::move does not invalidate the raw pointer
    while (it != in.end()
           && (*it)->getType() != CommandType::JUMP
           && (*it)->getType() != CommandType::CONDJUMP)
    {
        if ((*it)->getType() == CommandType::PUSH)
        {
            intraNodeStack.push(it->get());
            instrs.push_back(move(*it));
        }

        else if ((*it)->getType() == CommandType::POP && !intraNodeStack.empty())
        {
            PopCommand* pc = static_cast<PopCommand*>(it->get());
            if (!pc->isEmpty()) instrs.emplace_back(make_unique<AssignVarCommand>
                                                            (pc->getVarWrapper()->clone(),
                                                             intraNodeStack.top()->getAtom(),
                                                             intraNodeStack.top()->getLineNum()));
            intraNodeStack.pop();
        }
        else instrs.push_back(move(*it));
        ++it;
    }

    if (it == in.cend()) return;

    if ((*it)->getType() == CommandType::CONDJUMP)
    {
        JumpOnComparisonCommand* jocc = static_cast<JumpOnComparisonCommand*>(it->get());
        compSuccess = parentGraph.getNode(jocc->getString());
        if (compSuccess == nullptr) compSuccess = parentGraph.createNode(jocc->getString(), false, false);
        compSuccess->addParent(this);
        setComp(make_unique<JumpOnComparisonCommand>(*jocc));

        if (++it == in.cend()) return;
    }

    if ((*it)->getType() == CommandType::JUMP)
    {
        string jumpto = (*it)->getString();
        jumpline = (*it)->getLineNum();
        if (jumpto == "return") compFail = nullptr;
        else
        {
            compFail = parentGraph.getNode(jumpto);
            if (compFail == nullptr) compFail = parentGraph.createNode(jumpto, false, false);
            compFail->addParent(this);
        }
        if (++it != in.cend()) throw std::runtime_error("Should end here");
    }
    else throw std::runtime_error("Can only end w/ <=2 jumps");
}

void CFGNode::setFunctionCall(FunctionSymbol* fc)
{
    calledFunctionSymbol = fc;
}

bool CFGNode::addParent(CFGNode* parent)
{
    auto it = predecessors.find(parent->getName());

    if (it != predecessors.end()) return false;
    predecessors[parent->getName()] = parent;
    return true;
}

void CFGNode::removeParent(CFGNode* leaving)
{
    removeParent(leaving->getName());
}

void CFGNode::removeParent(const string& s)
{
    if (predecessors.erase(s) == 0) runtime_error("Parent '" + s + "' not found in '" + getName() + "'");
}

JumpOnComparisonCommand* CFGNode::getComp() const
{
    return comp.get();
}

void CFGNode::clearPredecessors()
{
    predecessors.clear();
}

const unordered_map<string, CFGNode*>& CFGNode::getPredecessorMap()
{
    return predecessors;
}

vector<CFGNode*> CFGNode::getSuccessorVector() const
{
    std::vector<CFGNode*> successors;
    if (getCompSuccess() != nullptr) successors.push_back(getCompSuccess());
    if (getCompFail() != nullptr) successors.push_back(getCompFail());
    else
    {
        if (!isLastNode()) throw std::runtime_error("only last node can return");
        for (const auto& retSucc : getParentFunction()->getNodesReturnedTo()) successors.push_back(retSucc);
    }
    return successors;
}
std::vector<CFGNode*> CFGNode::getPredecessorVector() const
{
    std::vector<CFGNode*> predecessorVec;
    for (const auto& pair : predecessors) predecessorVec.push_back(pair.second);
    return predecessorVec;
}

CFGNode* CFGNode::getCompSuccess() const
{
    return compSuccess;
}

CFGNode* CFGNode::getCompFail() const
{
    return compFail;
}

vector<unique_ptr<AbstractCommand>>& CFGNode::getInstrs()
{
    return instrs;
}

void CFGNode::setCompSuccess(CFGNode* compsucc)
{
    compSuccess = compsucc;
    if (compSuccess != nullptr)
    {
        if (comp == nullptr) throw std::runtime_error("comp should be set");
        compSuccess->addParent(this);
        comp->setString(compSuccess->getName());
    }
    else comp = nullptr;
}

bool CFGNode::noPreds()
{
    return (predecessors.empty() ||
            (predecessors.size() == 1 && predecessors.begin()->first == name));
}

FunctionSymbol* CFGNode::calledFunction()
{
    return calledFunctionSymbol;
}

void CFGNode::setCompFail(CFGNode* compareFail)
{
    compFail = compareFail;
    if (compFail != nullptr) compFail->addParent(this);
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
        throw std::runtime_error("must be function last");
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

unsigned int CFGNode::getNumPushingStates()
{
    return pushingStates.size();
}

void CFGNode::addFunctionCall(CFGNode* cfgn, FunctionSymbol *fs)
{
    if (!pushingStates.insert({cfgn, fs}).second) throw runtime_error("already in");
}

void CFGNode::removePushes()
{
    auto it = pushingStates.begin();
    while (it != pushingStates.end())
    {
        auto& firstInstrs = it->first->getInstrs();
        if (!it->first->calledFunction()) throw std::runtime_error("not right");
        unsigned int i = 0;
        bool done = false;
        while (i < firstInstrs.size())
        {
            if (firstInstrs[i]->getType() == CommandType::PUSH)
            {
                PushCommand* pc = static_cast<PushCommand*>(firstInstrs[i].get());
                if (pc->pushesState()) //found it
                {
                    if (pc->pushedVars > i) throw std::runtime_error("not enough pushes");
                    pc->calledFunction->forgetFunctionCall(it->first->getName(), name);

                    pc->calledFunction = nullptr;
                    it->first->setFunctionCall(nullptr);
                    firstInstrs.erase(firstInstrs.begin() + (i - pc->pushedVars - 1), firstInstrs.begin() + i + 1);
                    done = true;
                    break;
                }
            }
            ++i;
        }
        if (!done) throw runtime_error("couldnt find state push");
        it = pushingStates.erase(it);
    }
}

void CFGNode::replacePushes(const std::string& other)
{
    CFGNode* toReplaceWith = parentGraph.getNode(other);
    if (toReplaceWith == nullptr) throw runtime_error("asked to replace w/ nonexistent node");

    for (const auto& ps : pushingStates)
    {
        ps.second->replaceReturnState(this, toReplaceWith);
    }
    pushingStates.clear();
}

void CFGNode::removeFunctionCall(const string& bye, FunctionSymbol* fs)
{
    auto it = find_if(pushingStates.begin(), pushingStates.end(),
    [&, bye](std::pair<CFGNode*, FunctionSymbol*> p)
    {
        return bye == p.first->getName() && fs->getIdent() == p.second->getIdent();
    });
    if (it != pushingStates.end()) pushingStates.erase(it);
    else throw std::runtime_error("couldnt find call");
}

void CFGNode::prepareToDie()
{
    if (isLastNode() || name == parentGraph.getLast()->getName()) throw std::runtime_error("cant delete last node");
    if (getCompFail() != nullptr) getCompFail()->removeParent(name);
    if (getCompSuccess() != nullptr) getCompSuccess()->removeParent(name);

    removePushes();

    //find out if we push states
    for (const auto& ac : instrs)
    {
        if (ac->getType() == CommandType::PUSH)
        {
            auto pc = static_cast<PushCommand*>(ac.get());
            if (pc->pushesState())
            {
                const string& nodename = pc->getString();
                CFGNode* pushedNode = parentGraph.getNode(nodename);
                if (!pushedNode) throw std::runtime_error("could not find node");
                pc->calledFunction->removeFunctionCall(name, nodename, false);
                pushedNode->removeFunctionCall(name, pc->calledFunction);
            }
        }
    }
}