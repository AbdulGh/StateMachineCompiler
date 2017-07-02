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
    instrs = move(in);
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