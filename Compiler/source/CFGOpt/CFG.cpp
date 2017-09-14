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
    for (shared_ptr<AbstractCommand>& ac: instrs) outs << ac->translation();
    if (compSuccess != nullptr) outs << comp->translation();
    if (compFail != nullptr) outs << JumpCommand(compFail->getName(), jumpline).translation();
    else outs << ReturnCommand(jumpline).translation();
    outs << "end" << '\n';
    return outs.str();
}

void CFGNode::constProp()
{
    std::unordered_map<std::string,std::string> assignments;

    vector<std::shared_ptr<AbstractCommand>>::iterator it = instrs.begin();
    vector<std::shared_ptr<AbstractCommand>> newInstrs;

    while (it != instrs.end())
    {
        shared_ptr<AbstractCommand> current = std::move(*it);
        if (current->getType() == CommandType::CHANGEVAR)
        {
            assignments.erase(current->getData());
            newInstrs.push_back(current);
        }
        else if (current->getType() == CommandType::ASSIGNVAR)
        {
            shared_ptr<AssignVarCommand> avc = static_pointer_cast<AssignVarCommand>(current);
            string d1 = avc->getData();
            string d2 = avc->RHS;
            if (AbstractCommand::getStringType(avc->RHS) != AbstractCommand::StringType::ID)
            {
                string d1 = avc->getData();
                string d2 = avc->RHS;
                assignments[avc->getData()] = avc->RHS;
            }
            else
            {
                std::unordered_map<std::string, std::string>::const_iterator constit = assignments.find(avc->RHS);
                if (constit != assignments.end())
                {
                    assignments[avc->getData()] = constit->second;
                    avc->RHS = constit->second;
                    int debug;
                    debug = 2;
                }
                else
                {
                    assignments[avc->getData()] = avc->RHS;
                    int debug;
                    debug = 5;
                }
            }
            newInstrs.push_back(current);
        }
        else if (current->getType() == CommandType::EXPR)
        {
            shared_ptr<EvaluateExprCommand> eec = static_pointer_cast<EvaluateExprCommand>(current);
            //literals will not be found
            std::unordered_map<std::string, std::string>::const_iterator t1it = assignments.find(eec->term1);
            std::unordered_map<std::string, std::string>::const_iterator t2it = assignments.find(eec->term2);
            if (t1it != assignments.end()) eec->term1 = t1it->second;
            if (t2it != assignments.end()) eec->term2 = t2it->second;
            if (AbstractCommand::getStringType(eec->term1) != AbstractCommand::StringType::ID
                && AbstractCommand::getStringType(eec->term2) != AbstractCommand::StringType::ID)
            {
                try
                {
                    double lhs = stod(t1it->second);
                    double rhs = stod(t2it->second);
                    double result = evaluateOp(lhs, eec->op, rhs);
                    string resultstr = to_string(result);
                    assignments[eec->getData()] = resultstr;
                    int debug;
                    debug = 2;
                    newInstrs.push_back(make_shared<AssignVarCommand>(eec->getData(), resultstr, eec->getLineNum()));
                }
                catch (invalid_argument e)
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
        else newInstrs.push_back(current);
        it++;
    }
    instrs = move(newInstrs);
    if (comp != nullptr)
    {
        if (comp->term1Type == AbstractCommand::StringType::ID &&
            assignments.find(comp->term1) != assignments.end()) comp->setTerm1(assignments[comp->term1]);
        if (comp->term2Type == AbstractCommand::StringType::ID &&
            assignments.find(comp->term2) != assignments.end()) comp->setTerm2(assignments[comp->term2]);
        comp->makeGood();
    }
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

void CFGNode::setInstructions(vector<std::shared_ptr<AbstractCommand>>& in)
{
    instrs.clear();
    vector<std::shared_ptr<AbstractCommand>>::iterator it = in.begin();

    while (it != in.end()
           && (*it)->getType() != CommandType::JUMP
           && (*it)->getType() != CommandType::CONDJUMP)
    {
        std::shared_ptr<AbstractCommand> current = move(*it);
        instrs.push_back(current);
        ++it;
    }

    if (it == in.cend())
    {
        return;
    }

    if ((*it)->getType() == CommandType::CONDJUMP)
    {
        compSuccess = parent.getNode((*it)->getData());
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
            compFail = parent.getNode(jumpto);
            compFail->addParent(shared_from_this());
        }
        if (++it != in.cend()) throw "Should end here";
    }
    else throw "Can only end w/ <=2 jumps";
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
        vector<std::shared_ptr<AbstractCommand>> newInstrs = instrs;
        vector<std::shared_ptr<AbstractCommand>>& addingInstrs = other->getInstrs();
        newInstrs.reserve(instrs.size() + addingInstrs.size() + 2);
        for (shared_ptr<AbstractCommand>& newInst : addingInstrs) newInstrs.push_back(newInst->clone());
        setInstructions(newInstrs);
        if (other->getComp() != nullptr)
        {
            setComp(static_pointer_cast<JumpOnComparisonCommand>(other->getComp()->clone()));
        }
        else setComp(nullptr);
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

void ControlFlowGraph::addNode(std::string name, std::vector<std::shared_ptr<AbstractCommand>>& instrs)
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