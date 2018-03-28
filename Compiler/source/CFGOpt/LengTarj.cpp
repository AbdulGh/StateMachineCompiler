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
    unsigned long n = 0;
    function<void(CFGNode*, unsigned long)> DFS =
    [this, &DFS, &n](CFGNode* node, unsigned long parent) -> void
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

vector<unique_ptr<Loop>> LengTarj::findNesting(std::vector<unique_ptr<Loop>>& loops)
{
    unsigned int numLoops = loops.size();
    unsigned int numCFGNodes = controlFlowGraph.getCurrentNodes().size();
    
    bool** bitVectors = new bool*[numLoops];
    for (int i = 0; i < numLoops; ++i)
    {
        bitVectors[i] = new bool[numCFGNodes]();
        for (auto pair : loops[i]->getNodes())
        {
            CFGNode* node = pair.first;
            bitVectors[i][labels[node] - 1] = true;
        }
    }

    auto radixSort = //[l, r)
    [&, this, bitVectors](const auto& recurse, unsigned int digit, unsigned int l, unsigned int r) -> void
    {
        if (digit >= numCFGNodes || l >= r) return;
        unsigned int firstZero = l;

        for (unsigned int i = l; i < r; ++i)
        {
            if (bitVectors[i][digit] == false)
            {
                firstZero = i;
                break;
            }
        }

        unsigned int probe = firstZero + 1;
        while (probe < r)
        {
            if (bitVectors[probe][digit] == true)
            {
                bool* temp = bitVectors[probe];
                bitVectors[probe] = bitVectors[firstZero];
                bitVectors[firstZero] = temp;
                iter_swap(loops.begin() + probe, loops.begin() + firstZero);
                ++firstZero;
            }
            ++probe;
        }

        ++digit;
        recurse(recurse, digit, l, firstZero);
        recurse(recurse, digit, firstZero, r);
    };

    radixSort(radixSort, 0, 0, numLoops);

    vector<unique_ptr<Loop>> unnested;

    for (int i = numLoops - 1; i >= 0; --i)
    {
        for (int j = 0; j < numCFGNodes; ++j)
        {
            if (bitVectors[i][j] == true)
            {
                if (i == 1)
                {
                    bool debug2 = bitVectors[0][1];
                    int debug;
                    debug = 2;
                }

                //scan left until we hit another 1
                int scan = i - 1;
                while (scan >= 0 && bitVectors[scan][j] == false) --scan;

                if (scan >= 0)
                {
                    while (j < numCFGNodes)
                    {
                        if (bitVectors[i][j] == true)
                        {
                            if (bitVectors[scan][j] == false) throw runtime_error("not laminar");
                            CFGNode* shared =  verticies[j + 1]->node;
                            //possibly should check this is nullptr?
                            loops[scan]->setNodeNesting(shared, loops[i].get());
                        }
                        ++j;
                    }
                    loops[scan]->addChild(move(loops[i]));
                }
                else unnested.push_back(move(loops[i]));
                break;
            }
        }
    }
    return unnested;
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

vector<unique_ptr<Loop>> LengTarj::findLoops()
{
    labelNodes();
    calculateSemidominators();

    //calculate dominators and find `natural loops'
    vector<unique_ptr<Loop>> loops;
    for (unsigned long i = 2; i <= numNodes; ++i)
    {
        if (domNums[i] != semiDomNums[i]) domNums[i] = domNums[domNums[i]];
        std::vector<unsigned long>& iSuccessors = verticies[i]->successors;
        std::vector<unsigned long> natLoopTails;

        //climb up the dominator tree
        unsigned long currentDom = domNums[i];
        do
        {
            auto succ = find(iSuccessors.begin(), iSuccessors.end(), currentDom); //check if successor is a dominator
            if (succ != iSuccessors.end()) natLoopTails.push_back(currentDom);  //natural loop found
            currentDom = domNums[currentDom];
        } while (currentDom != domNums[currentDom]);

        for (const auto& succNum : natLoopTails)
        {
            stack<unsigned long> toProcess({i});

            std::map<CFGNode*, Loop*> nodeMap({{verticies[i]->node, nullptr}});
            while (!toProcess.empty()) //search upwards
            {
                unsigned long processingIndex = toProcess.top();
                toProcess.pop();
                if (processingIndex == succNum) continue;
                unique_ptr<NodeWrapper>& processing = verticies[processingIndex];
                for (unsigned long pred : processing->predecessors)
                {
                    unique_ptr<NodeWrapper>& predNode = verticies[pred];
                    if (nodeMap.insert({predNode->node, nullptr}).second) toProcess.push(pred);
                }
            }
            loops.push_back(make_unique<Loop>(verticies[succNum]->node, verticies[i]->node, move(nodeMap), succNum, controlFlowGraph));
        }
    }
    
    if (!loops.empty()) loops = findNesting(loops);
    return loops;
}