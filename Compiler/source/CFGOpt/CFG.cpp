#include <algorithm>
#include <map>
#include <set>
#include <memory>

#include "../compile/FunctionCodeGen.h"
#include "CFG.h"

using namespace std;

/*nodes*/
const string &CFGNode::getName() const
{
    return name;
}

string CFGNode::getSource()
{
    std::stringstream outs;

    outs << name << '\n';
    for (shared_ptr<AbstractCommand> ac: instrs) outs << ac->translation();
    if (compSuccess != nullptr) outs << comp->translation();
    if (compFail != nullptr) outs << JumpCommand(compFail->getName(), jumpline).translation();
    else outs << ReturnCommand(jumpline).translation();
    outs << "end" << '\n';
    return outs.str();
}

bool CFGNode::addParent(std::shared_ptr<CFGNode> parent)
{
    auto it = predecessors.find(parent->getName());
    if (it != predecessors.end()) return false;
    predecessors[parent->getName()] = parent;
    return true;
}

void CFGNode::removeParent(std::shared_ptr<CFGNode> leaving)
{
    removeParent(leaving->getName());
}

void CFGNode::removeParent(const std::string& s)
{
    if (predecessors.erase(s) == 0) throw runtime_error("Parent is not in");
}

void CFGNode::setInstructions(const vector<std::shared_ptr<AbstractCommand>> &in, bool constProp)
{
    instrs.clear();
    vector<std::shared_ptr<AbstractCommand>>::const_iterator it = in.cbegin();

    //first used to track assignments
    std::unordered_map<std::string,std::string> assignments;
    while (it != in.cend()
           && (*it)->getType() != CommandType::JUMP
           && (*it)->getType() != CommandType::CONDJUMP)
    {
        std::shared_ptr<AbstractCommand> current = *it;

        if (constProp)
        {
            if (current->getType() == CommandType::CHANGEVAR)
            {
                assignments.erase(current->getData());
                instrs.push_back(current);
            }
            else if (current->getType() == CommandType::ASSIGNVAR)
            {
                shared_ptr<AssignVarCommand> avc = static_pointer_cast<AssignVarCommand>(current);
                if (AbstractCommand::getStringType(avc->RHS) != AbstractCommand::StringType::ID) assignments[avc->getData()] = avc->RHS;
                else
                {
                    std::unordered_map<std::string,std::string>::const_iterator constit = assignments.find(avc->RHS);
                    if (constit != assignments.end())
                    {
                        assignments[avc->getData()] = constit->second;
                        avc->RHS = constit->second;
                    }
                    else assignments[avc->getData()] = avc->RHS;
                }
                instrs.push_back(current);
            }
            else if (current->getType() == CommandType::EXPR)
            {
                shared_ptr<EvaluateExprCommand> eec = static_pointer_cast<EvaluateExprCommand>(current);

                string lhs = eec->getData();
                string t1 = eec->term1;
                Op op = eec->op;
                string t2 = eec->term2;
                AbstractCommand::StringType t1t = AbstractCommand::getStringType(eec->term1);
                AbstractCommand::StringType t2t = AbstractCommand::getStringType(eec->term2);

                //literals will not be found
                std::unordered_map<std::string,std::string>::const_iterator t1it = assignments.find(eec->term1);
                std::unordered_map<std::string,std::string>::const_iterator t2it = assignments.find(eec->term2);
                if (t1it != assignments.end() && t2it != assignments.end())
                {
                    if (AbstractCommand::getStringType(eec->term1) != AbstractCommand::StringType::ID
                        && AbstractCommand::getStringType(eec->term1) != AbstractCommand::StringType::ID)
                    {

                        instrs.push_back(current);
                        try
                        {
                            double lhs = stod(t1it->second);
                            double rhs = stod(t2it->second);
                            double result = evaluateOp(lhs, eec->op, rhs);
                            string resultstr = to_string(result);
                            assignments[eec->getData()] = resultstr;
                            instrs.push_back(make_shared<AssignVarCommand>(eec->getData(), resultstr, eec->getLineNum()));
                        }
                        catch (invalid_argument e)
                        {
                            if (eec->op == PLUS)
                            {
                                string result = eec->term1 + eec->term2;
                                assignments[eec->getData()] = result;
                                instrs.push_back(make_shared<AssignVarCommand>(eec->getData(), result, eec->getLineNum()));
                            }
                            else throw runtime_error("Strings only support +");
                        }
                    }
                }
                else if (t1it != assignments.end())
                {
                    eec->term1 = t1it->second;
                    instrs.push_back(current);
                }
                else if (t2it != assignments.end())
                {
                    eec->term2 = t2it->second;
                    instrs.push_back(current);
                }
                else instrs.push_back(current);
            }
            else instrs.push_back(current);
        }
        else instrs.push_back(current);
        it++;
    }

    if (it == in.cend())
    {
        return;
    }

    //std::set<string> usedVariables;

    if ((*it)->getType() == CommandType::CONDJUMP)
    {
        comp = static_pointer_cast<JumpOnComparisonCommand>(*it);
        if (comp->term1Type == AbstractCommand::StringType::ID &&
            assignments.find(comp->term1) != assignments.end()) comp->setTerm1(assignments[comp->term1]);
        if (comp->term2Type == AbstractCommand::StringType::ID &&
            assignments.find(comp->term2) != assignments.end()) comp->setTerm2(assignments[comp->term2]);
        comp->makeGood();
        //if (comp->term1Type == AbstractCommand::StringType::ID) usedVariables.insert(comp->term1);
        //if (comp->term2Type == AbstractCommand::StringType::ID) usedVariables.insert(comp->term2);

        compSuccess = parent.getNode((*it)->getData());
        compSuccess->addParent(shared_from_this());

        if (++it == in.cend()) return;
    }

    if ((*it)->getType() == CommandType::JUMP)
    {
        string jumpto = (*it)->getData();
        jumpline = (*it)->getLineNum();
        if (jumpto == "return") compFail = nullptr;
        else
        {
            compFail = parent.getNode(jumpto);
            compFail->addParent(shared_from_this());
        }
        if (++it != in.cend()) throw "Should end here";
    }
    else throw "Can only end w/ <=2 jumps";

    /*
    std::vector<shared_ptr<AbstractCommand>>::reverse_iterator rit = instrs.rbegin();
    while (rit != instrs.rend())
    {
        std::shared_ptr<AbstractCommand> current = *rit;
        if (current->getType() == CommandType::CHANGEVAR
            && usedVariables.find(current->getData()) == usedVariables.end()) instrs.erase(--rit.base());
        else if (current->getType() == CommandType::ASSIGNVAR)
        {
            shared_ptr<AssignVarCommand> avc = static_pointer_cast<AssignVarCommand>(current);
            if (usedVariables.find(avc->getData()) == usedVariables.end()) instrs.erase(--rit.base());
            else usedVariables.insert(avc->RHS);
        }
        else if (current->getType() == CommandType::EXPR)
        {
            shared_ptr<EvaluateExprCommand> eec = static_pointer_cast<EvaluateExprCommand>(current);
            if (usedVariables.find(eec->getData()) == usedVariables.end()) instrs.erase(--rit.base());
            else
            {
                if (AbstractCommand::getStringType(eec->term1) == AbstractCommand::StringType::ID)
                    usedVariables.insert(eec->term1);
                if (AbstractCommand::getStringType(eec->term2) == AbstractCommand::StringType::ID)
                    usedVariables.insert(eec->term2);
            }
        }
        ++rit;
    }*/
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

vector<shared_ptr<AbstractCommand>> &CFGNode::getInstrs()
{
    return instrs;
}

void CFGNode::setCompSuccess(const shared_ptr<CFGNode> &compSuccess)
{
    CFGNode::compSuccess = compSuccess;
}

void CFGNode::setCompFail(const shared_ptr<CFGNode> &compFail)
{
    CFGNode::compFail = compFail;
}

void CFGNode::setComp(const shared_ptr<JumpOnComparisonCommand> &comp)
{
    CFGNode::comp = comp;
}

bool CFGNode::swallowNode(std::shared_ptr<CFGNode> other)
{
    if (compSuccess == nullptr && compFail != nullptr &&  compFail->getName() == other->getName())
    {
        /*shared_ptr<CFGNode> otherSucc = other->getCompSuccess();
        if (otherSucc != nullptr) otherSucc->addParent(shared_from_this());
        setCompSuccess(other->getCompSuccess());
        setComp(other->getComp());

        if (other->getCompFail() != nullptr) other->getCompFail()->addParent(shared_from_this());
        setCompFail(other->getCompFail());*/

        vector<std::shared_ptr<AbstractCommand>> newInstrs = instrs;
        vector<std::shared_ptr<AbstractCommand>>& addingInstrs = other->getInstrs();
        newInstrs.reserve(instrs.size() + addingInstrs.size() + 2);
        newInstrs.insert(newInstrs.end(), addingInstrs.begin(), addingInstrs.end());
        //if (other->getComp() != nullptr) newInstrs.push_back(other->getComp());
        //if (other->getCompFail() != nullptr) newInstrs.push_back
        //            (make_shared<JumpCommand>(other->getCompFail()->getName(), other->getJumpline()));
        //else newInstrs.push_back(make_shared<ReturnCommand>(other->getJumpline()));
        setInstructions(newInstrs);
        setComp(other->getComp());
        setCompSuccess(other->getCompSuccess());
        setCompFail(other->getCompFail());
        if (getCompSuccess() != nullptr) getCompSuccess()->addParent(shared_from_this());
        if (getCompFail() != nullptr) getCompFail()->addParent(shared_from_this());
        return true;
    }
    return false;
}

ControlFlowGraph &CFGNode::getParent() const
{
    return parent;
}

int CFGNode::getJumpline() const
{
    return jumpline;
}

/*graph*/

shared_ptr<CFGNode> ControlFlowGraph::getNode(string name, bool create)
{
    unordered_map<string, shared_ptr<CFGNode>>::const_iterator it = currentNodes.find(name);
    if (it == currentNodes.cend())
    {
        if (create)
        {
            shared_ptr<CFGNode> p = make_shared<CFGNode>(*this, name);
            currentNodes[name] = p;
            return p;
        }
        else return nullptr;
    }
    return it->second;
}

void ControlFlowGraph::removeNode(std::string name)
{
    if (last != nullptr && last->getName() == name)
    {
        if (last->getPredecessors().size() != 1) throw runtime_error("Can't replace last node");
        last = last->getPredecessors().cbegin()->second;
    }
    unordered_map<string, shared_ptr<CFGNode>>::iterator it = currentNodes.find(name);
    if (it == currentNodes.end()) throw "Check";
    currentNodes.erase(it);
}

void ControlFlowGraph::addNode(std::string name, std::vector<std::shared_ptr<AbstractCommand>> instrs)
{
    shared_ptr<CFGNode> introducing = getNode(name);
    if (currentNodes.size() == 1)  first = introducing;
    introducing->setInstructions(instrs);
}

std::string ControlFlowGraph::getSource()
{
    if (first == nullptr) return "";
    std::stringstream outs;
    outs << first->getSource() << '\n';

    for (auto it = currentNodes.cbegin(); it != currentNodes.cend(); it++)
    {
        if (it->first == first->getName()) continue;
        outs << it->second->getSource() << '\n';
    }

    return outs.str();
}

unordered_map<string, shared_ptr<CFGNode>> &ControlFlowGraph::getCurrentNodes()
{
    return currentNodes;
}

shared_ptr<CFGNode> ControlFlowGraph::getFirst() const
{
    return first;
}

shared_ptr<CFGNode> ControlFlowGraph::getLast() const
{
    return last;
}

void ControlFlowGraph::setLast(std::string lastName)
{
    unordered_map<string, shared_ptr<CFGNode>>::iterator it = currentNodes.find(lastName);
    if (it == currentNodes.end()) throw "Check";
    string debug = it->second->getName();
    last = it->second;
}