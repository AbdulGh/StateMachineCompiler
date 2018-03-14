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

string CFGNode::getSource(bool makeState, std::string delim, bool escape)
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
        if (!isLast) throw "should be last";
        for (CFGNode* nodePointer : parentFunction->getNodesReturnedTo())
        {
            outs << name << "->" << nodePointer->getName()
                 << "[label=\"return\"];\n";
        }
    }

    return outs.str();
}

bool CFGNode::constProp(unordered_map<string,string> assignments)
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
            case CommandType::CHANGEVAR:
            {
                assignments.erase(current->getData());
                newInstrs.push_back(move(current));
                break;
            }
            case CommandType::ASSIGNVAR:
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
                break;
            }
            case CommandType::EXPR:
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
    
                if (eec->term1 == eec->term2
                    && AbstractCommand::getStringType(eec->term1) == AbstractCommand::StringType::ID
                    && parentGraph.symbolTable.findIdentifier(eec->term1)->getType() == DOUBLE)
                {
                    switch (eec->op)
                    {
                        case MINUS:
                            assignments[eec->getData()] = "0";
                            newInstrs.push_back(make_unique<AssignVarCommand>(eec->getData(), "0", eec->getLineNum()));
                            break;
                        case PLUS:
                            assignments.erase(eec->getData());
                            eec->op = MULT;
                            eec->term2 = "2";
                            newInstrs.push_back(move(current));
                            break;
                        default:
                            assignments.erase(eec->getData());
                            newInstrs.push_back(move(current));
                    }
                }//when we leave this at least one term is an ID so we dont enter the next condition
    
                if (AbstractCommand::getStringType(eec->term1) != AbstractCommand::StringType::ID
                    && AbstractCommand::getStringType(eec->term2) != AbstractCommand::StringType::ID)
                {
                    try
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
                break;
            }
            case CommandType::PRINT:
            {
                unordered_map<string, string>::const_iterator t1it = assignments.find(current->getData());
                if (t1it != assignments.end()) current->setData(t1it->second);
                newInstrs.push_back(move(current));
                break;
            }
            case CommandType::PUSH:
            {
                auto pushc = static_cast<PushCommand*>(current.get());
                if (!pushc->pushesState())
                {
                    auto pushedVarIt = assignments.find(current->getData());
                    if (pushedVarIt != assignments.end()) current->setData(pushedVarIt->second);
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
                            CFGNode* node = parentGraph.getNode(pushc->getData());
                            if (node == nullptr) throw "found a bad state";
                            pushc->calledFunction->removeFunctionCall(name, pushc->getData());
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
                            if (current->getData() != pushc->getData())
                            {
                                newInstrs.push_back(make_unique<AssignVarCommand>
                                                            (current->getData(), pushc->getData(), current->getLineNum()));
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
        /*if (comp->term1.type == AbstractCommand::StringType::ID &&
            assignments.find(string(comp->term1)) != assignments.end()) comp->setTerm1(assignments[string(comp->term1)]);
        if (comp->term2.type == AbstractCommand::StringType::ID &&
            assignments.find(string(comp->term2)) != assignments.end()) comp->setTerm2(assignments[string(comp->term2)]);*/

        //check for const comparison
        if (comp->term1.type != AbstractCommand::StringType::ID
            && comp->term2.type != AbstractCommand::StringType::ID)
        {
            if (comp->term1.type != comp->term2.type)
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
                if (comp->term1.type == AbstractCommand::StringType::DOUBLELIT)
                {
                    double d1 = stod(string(comp->term1));
                    double d2 = stod(string(comp->term2));
                    isTrue = Relations::evaluateRelop<double>(d1, comp->op, d2);
                }
                else isTrue = Relations::evaluateRelop<string>(string(comp->term1), comp->op, string(comp->term2));

                if (isTrue)
                {
                    if (getCompFail() != nullptr) getCompFail()->removeParent(getName());
                    else throw "shouldnt happen at the end of a function call";
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
    if (other->getName() == name) throw "cant swallow self";

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
                        CFGNode* node = parentGraph.getNode(pc->getData());
                        if (node == nullptr) throw "pushing nonexistent node";
                        else if (calledFunctionSymbol && !needlessFunctionCall) throw "can only call one function";
                        if (!other->calledFunctionSymbol) throw "other is pushing states w/out calling";
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
                    comp->setData(compFail->getName());
                    compSuccess = compFail;
                    compFail = nullptr;
                }
            }
            else setCompSuccess(other->getCompFail());
        }

        else if (compFail != nullptr)
        {
            if (compFail->getName() != other->getName())  throw "should be compfail";
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

void CFGNode::appendDeclatation(VariableType type, std::string varName)
{
    instrs.push_back(make_unique<DeclareVarCommand>(type, move(varName), -1));
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
                                                            (pc->getData(), intraNodeStack.top()->getData(),
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
        compSuccess = parentGraph.getNode(jocc->getData());
        if (compSuccess == nullptr) compSuccess = parentGraph.createNode(jocc->getData(), false, false);
        compSuccess->addParent(this);
        setComp(make_unique<JumpOnComparisonCommand>(*jocc));

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
            compFail->addParent(this);
        }
        if (++it != in.cend()) throw "Should end here";
    }
    else throw "Can only end w/ <=2 jumps";
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
        if (!isLastNode()) throw "only last node can return";
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
        if (comp == nullptr) throw "comp should be set";
        compSuccess->addParent(this);
        comp->setData(compSuccess->getName());
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

unsigned int CFGNode::getNumPushingStates()
{
    return pushingStates.size();
}

void CFGNode::addFunctionCall(CFGNode* cfgn, FunctionSymbol *fs)
{
    if (!pushingStates.insert({cfgn, fs}).second) throw "already in";
}

void CFGNode::removePushes()
{
    auto it = pushingStates.begin();
    while (it != pushingStates.end())
    {
        auto& firstInstrs = it->first->getInstrs();
        if (!it->first->calledFunction()) throw "not right";
        unsigned int i = 0;
        bool done = false;
        while (i < firstInstrs.size())
        {
            if (firstInstrs[i]->getType() == CommandType::PUSH)
            {
                PushCommand* pc = static_cast<PushCommand*>(firstInstrs[i].get());
                if (pc->pushesState()) //found it
                {
                    if (pc->pushedVars > i) throw "not enough pushes";
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
        if (!done) throw "couldnt find state push";
        it = pushingStates.erase(it);
    }
}

void CFGNode::replacePushes(const std::string& other)
{
    CFGNode* toReplaceWith = parentGraph.getNode(other);
    if (toReplaceWith == nullptr) throw "asked to replace w/ nonexistent node";

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
    else throw "couldnt find call";
}

void CFGNode::prepareToDie()
{
    if (isLastNode() || name == parentGraph.getLast()->getName()) throw "cant delete last node";
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
                CFGNode* pushedNode = parentGraph.getNode(pc->getData());
                if (!pushedNode) throw "could not find node";
                pc->calledFunction->removeFunctionCall(name, pc->getData(), false);
                pushedNode->removeFunctionCall(name, pc->calledFunction);
            }
        }
    }
}