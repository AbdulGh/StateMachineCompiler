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

void CFGNode::setInstructions(const vector<std::shared_ptr<AbstractCommand>> &in)
{
    instrs.clear();
    vector<std::shared_ptr<AbstractCommand>>::const_iterator it = in.cbegin();

    std::unordered_map<std::string,std::string> constVariables;

    while (it != in.cend()
           && (*it)->getType() != CommandType::JUMP
           && (*it)->getType() != CommandType::CONDJUMP)
    {
        std::shared_ptr<AbstractCommand> current = *it;
        if (current->getType() == CommandType::ASSIGNVAR)
        {
            shared_ptr<AssignVarCommand> avc = static_pointer_cast<AssignVarCommand>(current);
            if (!(AbstractCommand::getStringType(avc->RHS) == AbstractCommand::StringType::ID
                    && constVariables.find(avc->RHS) == constVariables.end())) constVariables[avc->getData()] = avc->RHS;
            instrs.push_back(current);
        }
        else if (current->getType() == CommandType::EXPR)
        {
            shared_ptr<EvaluateExprCommand> eec = static_pointer_cast<EvaluateExprCommand>(current);

            //if either operand is a variable not known to be constant
            AbstractCommand::StringType t1type = AbstractCommand::getStringType(eec->term1);
            AbstractCommand::StringType t2type = AbstractCommand::getStringType(eec->term2);
            if (!((t1type == AbstractCommand::StringType::ID
               && constVariables.find(eec->term1) == constVariables.end()) ||
                ((t2type == AbstractCommand::StringType::ID
                  && constVariables.find(eec->term2) == constVariables.end()))))
            {
                string t1val = (t1type == AbstractCommand::StringType::ID) ? constVariables[eec->term1] : eec->term1;
                string t2val = (t2type == AbstractCommand::StringType::ID) ? constVariables[eec->term2] : eec->term2;
                try
                {
                    double lhs = stod(t1val);
                    double rhs = stod(t2val);
                    double result = evaluateOp(lhs, eec->op, rhs);
                    string resultstr = to_string(result);
                    constVariables[eec->getData()] = resultstr;
                    instrs.push_back(make_shared<AssignVarCommand>(eec->getData(), resultstr, eec->getLineNum()));
                }
                catch (invalid_argument e)
                {
                    if (eec->op == PLUS)
                    {
                        string result = eec->term1 + eec->term2;
                        constVariables[eec->getData()] = result;
                        instrs.push_back(make_shared<AssignVarCommand>(eec->getData(), result, eec->getLineNum()));
                    }
                    else throw runtime_error("Strings only support +");
                }

            }
        }
        else instrs.push_back(current);
        it++;
    }

    if (it == in.cend()) return;

    if ((*it)->getType() == CommandType::CONDJUMP)
    {
        comp = static_pointer_cast<JumpOnComparisonCommand>(*it);
        if (comp->term1Type == AbstractCommand::StringType::ID &&
            constVariables.find(comp->term1) != constVariables.end()) comp->setTerm1(constVariables[comp->term1]);
        if (comp->term2Type == AbstractCommand::StringType::ID &&
            constVariables.find(comp->term2) != constVariables.end()) comp->setTerm2(constVariables[comp->term2]);
        comp->makeGood();
        //if these are both const they get replaced during 1st symbolic execution todo warn about this

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
        shared_ptr<CFGNode> otherSucc = other->getCompSuccess();
        if (otherSucc != nullptr) otherSucc->addParent(shared_from_this());
        setCompSuccess(other->getCompSuccess());
        setComp(other->getComp());

        if (other->getCompFail() != nullptr) other->getCompFail()->addParent(shared_from_this());
        setCompFail(other->getCompFail());

        vector<std::shared_ptr<AbstractCommand>> newInstrs = instrs;
        vector<std::shared_ptr<AbstractCommand>>& addingInstrs = other->getInstrs();
        newInstrs.reserve(instrs.size() + addingInstrs.size());
        newInstrs.insert(newInstrs.end(), addingInstrs.begin(), addingInstrs.end());
        setInstructions(newInstrs);
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
    last = it->second;
}