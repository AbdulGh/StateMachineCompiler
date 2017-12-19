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
#include "../symbolic/SymbolicExecution.h"

//implemented in LoopValidation.cpp

typedef std::map<CFGNode*, std::map<std::string, unsigned short int>> ChangeMap;
typedef std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> SEFPointer;

struct Loop
{
private:
    typedef SymbolicExecution::SymbolicExecutionManager::SearchResult SearchResult;
    Reporter& reporter;
    CFGNode* headerNode;
    CFGNode* exitNode;
    std::set<CFGNode*> nodes;
    JumpOnComparisonCommand* comp;
    bool reverse = false;
    bool goodPathFound = false;

    //returns true if it was a node in the loop on a path to the loop exit
    bool searchNode(CFGNode* node, ChangeMap& map, std::unordered_map<std::string, std::unique_ptr<SearchResult>>& tags,
                    SEFPointer sef, std::string& badExample);

public:
    Loop(CFGNode* entry, CFGNode* last, std::set<CFGNode*> nodeSet, Reporter& r);
    std::string getInfo();
    std::set<CFGNode*> getNodeCopy() {return nodes;}
    void validate(std::unordered_map<std::string, std::unique_ptr<SearchResult>>& tags);
};

#endif //PROJECT_LOOP_H
