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

typedef std::map<std::string, unsigned short int> NodeChangeMap;
typedef std::map<CFGNode*, NodeChangeMap> ChangeMap;
typedef std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> SEFPointer;

class Loop
{
private:
    typedef SymbolicExecution::SymbolicExecutionManager::SearchResult SearchResult;
    ControlFlowGraph& cfg;
    CFGNode* headerNode;
    CFGNode* comparisonNode;
    std::map<CFGNode*, Loop*> nodes;
    std::set<std::unique_ptr<Loop>> children;
    bool stackBased = false;
    bool goodPathFound = false;
    bool invalid;
    const unsigned long domNum;

    //returns true if it was a node in the loop on a path to the loop exit
    bool searchNode(CFGNode* node, ChangeMap& map, std::unordered_map<std::string, std::unique_ptr<SearchResult>>& tags,
                    SEFPointer sef, std::string& badExample, bool headerSeen = true);


public:
    Loop(CFGNode* entry, CFGNode* last, std::map<CFGNode*, Loop*> nodeSet, unsigned long tailNumber, ControlFlowGraph& cfg);
    const std::map<CFGNode*, Loop*>& getNodes() {return nodes;}
    std::string getInfo(std::string indent = "|");
    void addChild(std::unique_ptr<Loop> child);
    void setNodeNesting(CFGNode* node, Loop* child);
    void validate(std::unordered_map<std::string, std::unique_ptr<SearchResult>>& tags);
};

#endif //PROJECT_LOOP_H
