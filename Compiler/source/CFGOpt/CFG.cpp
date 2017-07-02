#include "CFG.h"

using namespace std;

shared_ptr<CFGNode> ControlFlowGraph::createNodeIfNotExists(string name)
{
    unordered_map::const_iterator it = currentNodes.find(name);
    if (it == currentNodes.cend())
    {
        shared_ptr p(new CFGNode(*this, name));
        currentNodes[name] = p;
        return p;
    }
    return *it;
}