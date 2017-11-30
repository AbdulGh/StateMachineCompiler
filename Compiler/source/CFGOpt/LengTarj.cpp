//
// Created by abdul on 23/10/17.
//

#include <stack>
#include <algorithm>
#include <functional>

#include "LengTarj.h"
#include "Loop.h"

using namespace std;

LengTarj::LengTarj(ControlFlowGraph& cfg): controlFlowGraph(cfg), numNodes(cfg.getCurrentNodes().size())
{
    unsigned long actualNum = numNodes + 2; //we use zero for `none'
    verticies = new unique_ptr<NodeWrapper>[actualNum];
    domNums = new unsigned long[actualNum];
    semiDomNums = new unsigned long[actualNum](); //zero initialized
    domNums[0] = domNums[1] = 1;
    forestAncestors = new unsigned long[actualNum]();
    forestMinimums = new unsigned long[actualNum];
}

LengTarj::~LengTarj()
{
    delete[] verticies;
    delete[] domNums;
    delete[] semiDomNums;
    delete[] forestAncestors;
    delete[] forestMinimums;
}

void LengTarj::labelNodes()
{
    std::map<CFGNode*, unsigned long> labels;
    unsigned long n = 0;
    function<void(CFGNode*, unsigned long)> DFS =
    [this, &labels, &DFS, &n](CFGNode* node, unsigned long parent) -> void
    {
        ++n;
        labels[node] = n;
        semiDomNums[n] = n;
        forestMinimums[n] = n;
        domNums[n] = n; //might cause problems
        verticies[n] = make_unique<NodeWrapper>(node, parent, n);
        for (CFGNode* succ : node->getSuccessorVector()) if (labels.find(succ) == labels.end()) DFS(succ, labels[node]);
    };
    DFS(controlFlowGraph.getFirst(), 0);
    numNodes = n;

    //populate pred/succ arrays
    for (unsigned long i = 1; i <= numNodes; ++i)
    {
        unique_ptr<NodeWrapper>& wrapperPointer = verticies[i];
        vector<CFGNode*> predVec = wrapperPointer->node->getPredecessorVector();
        wrapperPointer->predecessors.reserve(predVec.size());
        for (auto pred : predVec) if (labels.find(pred) != labels.end())
        {
            wrapperPointer->predecessors.push_back(labels[pred]);
        }

        vector<CFGNode*> succVec = wrapperPointer->node->getSuccessorVector();
        wrapperPointer->successors.reserve(succVec.size());
        for (auto succ : succVec) if (labels.find(succ) != labels.end())
        {
            if (succ->getName() == wrapperPointer->node->getName())
            {
                wrapperPointer->selfLoop = true;
                domNums[i] = semiDomNums[i] = i;
            }
            wrapperPointer->successors.push_back(labels[succ]);
        }
    }
}

void LengTarj::calculateSemidominators()
{
    //work in reverse preorder, skip first node
    for (unsigned long currentNode = numNodes; currentNode > 1; --currentNode)
    {
        //semidominator is minimum of semidominator of predecessors
        //note that lowered numbered predecessors have their semidominators currently labled as themselves
        unique_ptr<NodeWrapper>& wrapperPointer = verticies[currentNode];
        if (wrapperPointer->selfLoop) continue; //already done
        unsigned long minSemidominator = numNodes;
        for (unsigned long pred : wrapperPointer->predecessors)
        {
            unsigned long predSemi = semiDomNums[eval(pred)];
            if (predSemi < minSemidominator) minSemidominator = predSemi;
        }
        semiDomNums[currentNode] = minSemidominator;
        buckets[minSemidominator].push_back(currentNode);

        //link currentNode
        unsigned long parent = verticies[currentNode]->DFSParent;
        forestAncestors[currentNode] = parent;

        //all the nodes semidominated by currentNodes parent are ready to be properly processed
        auto parentIt = buckets.find(parent);
        if (parentIt != buckets.end())
        {
            vector<unsigned long>& semidominatedNodes = parentIt->second;
            for (unsigned long semidominated : semidominatedNodes)
            {
                unsigned long minSemidominatedOnPath = eval(semidominated);
                if (semiDomNums[semidominated] == semiDomNums[minSemidominatedOnPath])
                {
                    domNums[semidominated] = semiDomNums[semidominated];
                }
                else domNums[semidominated] = minSemidominatedOnPath;
            }
            buckets.erase(parent);
        }
    }
}

unsigned long LengTarj::eval(unsigned long node)
{
    if (forestAncestors[node] == 0) return node;
    else return compress(node);
}

unsigned long LengTarj::compress(unsigned long node)
{
    if (forestAncestors[forestAncestors[node]] != 0) //if node is the root or a direct decendent of one we can do no compression
    {
        unsigned long ancestor = forestAncestors[node];
        unsigned long parentMinimum = compress(ancestor);
        if (parentMinimum != forestMinimums[node]) forestMinimums[node] = parentMinimum;
        forestAncestors[node] = forestAncestors[ancestor];
    }
    return forestMinimums[node];
}

vector<Loop> LengTarj::findLoops()
{
    labelNodes();
    calculateSemidominators();
    vector<Loop> loops;
    //calculate dominators and find `natural loops'
    for (unsigned long i = 2; i <= numNodes; ++i)
    {
        if (domNums[i] != semiDomNums[i]) domNums[i] = domNums[domNums[i]];
        std::vector<unsigned long>& iSuccessors = verticies[i]->successors;
        auto succ = find(iSuccessors.begin(), iSuccessors.end(), domNums[i]); //check if successor is immediate dominator
        if (succ != iSuccessors.end()) //natural loop found
        {
            unsigned long succNum = *succ;
            stack<unsigned long> toProcess({i});
            std::set<CFGNode*> nodeSet({verticies[i]->node});
            while (!toProcess.empty()) //search upwards
            {
                unsigned long processingIndex = toProcess.top();
                toProcess.pop();
                if (processingIndex == succNum) continue;
                unique_ptr<NodeWrapper>& processing = verticies[processingIndex];
                for (unsigned long pred : processing->predecessors)
                {
                    unique_ptr<NodeWrapper>& predNode = verticies[pred];
                    if (nodeSet.insert(predNode->node).second) toProcess.push(pred);
                }
            }
            loops.emplace_back(Loop(verticies[succNum]->node, verticies[i]->node, nodeSet, controlFlowGraph.getReporter()));
        }
    }
    return loops;
}