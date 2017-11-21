//
// Created by abdul on 26/10/17.
//
//validation stuff in symbolic/LoopValidation.cpp

#ifndef PROJECT_LOOP_H
#define PROJECT_LOOP_H

#include <map>

#include "CFG.h"
#include "../symbolic/SymbolicVarSet.h"
#include "../Command.h"

//validation stuff in loopvalidation.cpp

typedef std::map<CFGNode*, std::map<std::string, unsigned short int>> ChangeMap;
typedef std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> SEFPointer;

struct Loop
{
private:
    Reporter& reporter;
    CFGNode* headerNode;
    CFGNode* exitNode;
    std::set<CFGNode*> nodes;
    JumpOnComparisonCommand* comp;
    bool goodPathFound = false;
    //std::string badExample = "";

    //returns true if it was a node in the loop on a path to the loop exit
    bool searchNode(CFGNode* node, ChangeMap& map, SEFPointer sef, std::string& badExample);

public:
    Loop(CFGNode* entry, CFGNode* last, std::set<CFGNode*> nodeSet, Reporter& r):
            headerNode(entry), exitNode(last), nodes(move(nodeSet)), reporter(r)
    {
        if (exitNode->getCompSuccess() != nullptr
            && exitNode->getCompSuccess()->getName() == headerNode->getName()) comp = exitNode->getComp();
        else throw "should be that";
    }

    std::string getInfo()
    {
        std::string outStr = "Header: " + headerNode->getName() + "\nExit: " + exitNode->getName() + "\nNodes:\n";
        for (CFGNode* node: nodes) outStr += node->getName() + "\n";
        return outStr;
    }

    std::set<CFGNode*> getNodeCopy() {return nodes;}

    void validate(std::unordered_map<std::string, std::shared_ptr<SymbolicVarSet>>& tags);
};

#endif //PROJECT_LOOP_H
