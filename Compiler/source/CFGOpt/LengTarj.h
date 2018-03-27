#ifndef PROJECT_LENGTARJ_H
#define PROJECT_LENGTARJ_H

/*
 * Original paper: `A fast algorithm for finding dominators in a flowgraph'
 * Implementation paper (followed loosely): `Computing Dominance and Dominance Frontiers'
 */

#include <map>

#include "CFG.h"
#include "Loop.h"

class LengTarj
{
private:
    ControlFlowGraph& controlFlowGraph;

    struct NodeWrapper
    {
        unsigned long number;
        unsigned long DFSParent;
        bool selfLoop = false;
        CFGNode* node;

        std::vector<unsigned long> predecessors;
        std::vector<unsigned long> successors;

        NodeWrapper(CFGNode* n, unsigned long parent, unsigned long label):
                node(n), DFSParent(parent), number(label) {}
    };

    unsigned long numNodes;

    std::unique_ptr<NodeWrapper>* verticies; //number -> node
    unsigned long* semiDomNums;
    unsigned long* domNums;

    //forest stuff
    unsigned long* forestAncestors;
    unsigned long* forestMinimums;
    std::map<CFGNode*, unsigned long> labels;
    unsigned long compress(unsigned long node);
    unsigned long eval(unsigned long node);
    std::map<unsigned long, std::vector<unsigned long>> buckets;

    void labelNodes();
    void calculateSemidominators();
    std::vector<Loop> findNesting(std::vector<std::unique_ptr<Loop>>& loops);
public:
    LengTarj(ControlFlowGraph& cfg);
    std::vector<std::unique_ptr<Loop>> findLoops();
    ~LengTarj();
};


#endif