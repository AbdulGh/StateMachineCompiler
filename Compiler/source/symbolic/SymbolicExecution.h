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
template <typename T>
using VarTemplatePointer = std::shared_ptr<SymbolicVariableTemplate<T>>;

namespace SymbolicExecution
{
    struct Condition
    {
        std::string nodeName;
        std::string l;
        Relations::Relop c;
        std::string r;

        Condition(std::string lhs, Relations::Relop comp, std::string rhs):
                l(lhs), c(comp), r(rhs) {}
        Condition(std::string lhs, Relations::Relop comp, double rhs):
                l(lhs), c(comp), r(std::to_string(rhs)) {}
    };

    class SymbolicExecutionFringe
    {
    private:
        bool feasable = true;
        std::shared_ptr<SymbolicExecutionFringe> parent;

    public:
        SymbolicExecutionFringe(Reporter& r);
        SymbolicExecutionFringe(std::shared_ptr<SymbolicExecutionFringe> p);

        std::unordered_map<std::string, std::shared_ptr<Condition>> pathConditions;
        std::shared_ptr<SymbolicStack> currentStack;
        std::shared_ptr<SymbolicVarSet> symbolicVarSet;
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

        //returns if a feasable path extention goes through this node
        //a path is feasable if it visits itself or reaches the last state
        bool visitNode(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n);
        //all the below static cast to SymbolicVariableTemplate<T>
        //these don't check if the ranges are disjoint - this is done in visitNode
        template <typename T>
        bool branch(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n, std::string lhsvar,
                    Relations::Relop op, const T& rhsconst, bool reverse = false);
        template <typename T>
        bool branchEQ(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                      std::string lhsvar, const T& rhsconst, bool reverse = false);
        template <typename T>
        bool branchNE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                      std::string lhsvar, const T& rhsconst, bool reverse = false);
        template <typename T>
        bool branchLE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                      std::string lhsvar, const T& rhsconst, bool reverse = false);
        template <typename T>
        bool branchLT(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                      std::string lhsvar, const T& rhsconst, bool reverse = false);
        template <typename T>
        bool branchGE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                           std::string lhsvar, const T& rhsconst, bool reverse = false);
        template <typename T>
        bool branchGT(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                      std::string lhsvar, const T& rhsconst, bool reverse = false);
        template <typename T>
        bool varBranch(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                       VarTemplatePointer<T> lhsvar, Relations::Relop op, VarTemplatePointer<T> rhsvar);
        template <typename T>
        bool varBranchEQ(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                      VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar);
        template <typename T>
        bool varBranchNE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                      VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar);
        template <typename T>
        bool varBranchLE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                      VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar);
        template <typename T>
        bool varBranchLT(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                      VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar);
        template <typename T>
        bool varBranchGE(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                      VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar);
        template <typename T>
        bool varBranchGT(std::shared_ptr<SymbolicExecutionFringe> sef, std::shared_ptr<CFGNode> n,
                      VarTemplatePointer<T> lhsvar, VarTemplatePointer<T> rhsvar);
        template <typename T>
        VarTemplatePointer<T>& getGreatestLowerBound(VarTemplatePointer<T>& lhsvar, VarTemplatePointer<T>& rhsvar);
        template <typename T>
        VarTemplatePointer<T>& getLeastUpperBound(VarTemplatePointer<T>& lhsvar, VarTemplatePointer<T>& rhsvar);

        std::shared_ptr<CFGNode> getFailNode(std::shared_ptr<SymbolicExecutionFringe> returningSEF, std::shared_ptr<CFGNode> n);

    public:
        SymbolicExecutionManager(ControlFlowGraph& cfg, SymbolTable& sTable, Reporter& reporter):
            cfg(cfg), sTable(sTable), reporter(reporter)
        {
            for (const auto& pair : cfg.getCurrentNodes()) feasableVisits[pair.first] = 0;
        };
        void search();
        //void removeUnreachableStates();
    };
}

#endif