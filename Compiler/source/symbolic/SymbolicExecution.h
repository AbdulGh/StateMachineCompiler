#ifndef PROJECT_DFSSYMBOLICEXECUTION_H
#define PROJECT_DFSSYMBOLICEXECUTION_H

#include <unordered_map>
#include <stack>

#include "../CFGOpt/CFG.h"
#include "SymbolicVariables.h"
#include "SymbolicStack.h"
#include "SymbolicVarSet.h"
#include "../compile/Token.h" //relop

typedef std::shared_ptr<SymbolicVariable> VarPointer;

namespace SymbolicExecution
{
    struct Condition
    {
        std::string nodeName;
        std::string l;
        Relations::Relop c;
        std::string r;

        Condition(std::string lhs, Relations::Relop comp, std::string rhs):
                l(lhs), r(rhs), c(comp) {}
    };

    class SymbolicExecutionFringe
    {
    private:
        bool feasable = true;
        SymbolicExecutionFringe* parent;

    public:
        SymbolicExecutionFringe(Reporter& r, SymbolicExecutionFringe* p = nullptr);

        std::unordered_map<std::string, std::shared_ptr<Condition>> pathConditions;
        SymbolicStack currentStack;
        SymbolicVarSet symbolicVarSet;
        Reporter& reporter;

        void error(Reporter::AlertType a, std::string s, int linenum = -1);
        void warn(Reporter::AlertType a, std::string s, int linenum = -1);
        bool isFeasable();
        bool hasSeen(std::string state);
    };

    class SymbolicExecutionManager
    {
    private:
        std::unordered_map<std::string, int> feasableVisits;
        ControlFlowGraph& cfg;
        SymbolTable& sTable;
        Reporter& reporter;

        /*returns if a feasable path extention goes through this node
         * a path is feasable if it visits itself or reaches the last state*/
        bool visitNode(SymbolicExecutionFringe* sef, std::shared_ptr<CFGNode> n);

    public:
        SymbolicExecutionManager(ControlFlowGraph& cfg, SymbolTable& sTable, Reporter reporter):
            cfg(cfg), sTable(sTable), reporter(reporter)
        {
            for (const auto& pair : cfg.getCurrentNodes()) feasableVisits[pair.first] = 0;
        };
        void search();
        void removeUnreachableStates();
    };
}

#endif