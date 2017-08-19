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
        Relop c;
        std::string r;

        Condition(std::string lhs, Relop comp, std::string rhs):
                l(lhs), r(rhs), c(comp) {}
    };

    class SymbolicExecutionFringe
    {
    private:
        Reporter& reporter;
        bool feasable = true;

    public:
        std::vector<Condition> pathCondition;
        SymbolicStack currentStack;
        SymbolicVarSet symbolicDoubleSet;

        void error(Reporter::AlertType a, std::string s)
        {
            reporter.error(a, s);
            feasable = false;
        }

        bool checkFeasable()
        {
            if (!symbolicDoubleSet.isFeasable()) feasable = false;
            return feasable;
        }
    };

    class SymbolicExecution
    {
    private:
        std::unordered_map<std::string, int> feasableVisits;
        ControlFlowGraph& cfg;
        SymbolTable& sTable;

        /*returns if a feasable path extention goes through this node
         * a path is feasable if it visits itself or reaches the last state*/
        bool visitNode(std::shared_ptr<CFGNode> n);

    public:
        SymbolicExecution(ControlFlowGraph& cfg, SymbolTable& sTable):
            cfg(cfg), sTable(sTable) {};
        void search();
        void removeUnreachableStates();
    };
}

#endif