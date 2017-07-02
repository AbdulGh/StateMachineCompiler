#include "CFG.h"

using namespace std;

/*nodes*/
ControlFlowGraph::CFGNode::~CFGNode()
{
    if (comp != nullptr) delete(comp);
}

void ControlFlowGraph::CFGNode::addParent(std::shared_ptr<CFGNode> parent)
{
    incoming.push_back(parent);
}

void ControlFlowGraph::CFGNode::setInstructions(const vector<std::shared_ptr<AbstractCommand>> &in)
{
    vector<std::shared_ptr<AbstractCommand>>::const_iterator it = in.cbegin();

    while (it != in.cend() &&
            ((*it)->getEffect() != CommandSideEffect::JUMP || (*it)->getEffect() != CommandSideEffect::CONDJUMP))
    {
        instrs.push_back(*it);
    }

    if (it == in.cend()) return;

    if ((*it)->getEffect() == CommandSideEffect::CONDJUMP)
    {
        shared_ptr<JumpOnComparisonCommand> jocc = static_pointer_cast<JumpOnComparisonCommand>(*it);
        comp = new Comparison(jocc->term1, jocc->term2, jocc->op);
        compSuccess = parent.createNodeIfNotExists(jocc->getData());
        compSuccess->addParent(shared_from_this());

        if (++it == in.cend()) return;
    }

    if ((*it)->getEffect() == CommandSideEffect::JUMP)
    {
        compFail = parent.createNodeIfNotExists((*it)->getData());
        compFail->addParent(shared_from_this());
        if (++it != in.cend()) throw "Should end here";
    }
    else throw "Can only end w/ <=2 jumps";
}

shared_ptr<ControlFlowGraph::CFGNode> ControlFlowGraph::createNodeIfNotExists(string name)
{
    unordered_map<string, shared_ptr<ControlFlowGraph::CFGNode>>::const_iterator it = currentNodes.find(name);
    if (it == currentNodes.cend())
    {
        shared_ptr<ControlFlowGraph::CFGNode> p(new CFGNode(*this, name));
        currentNodes[name] = p;
        return p;
    }
    return it->second;
}