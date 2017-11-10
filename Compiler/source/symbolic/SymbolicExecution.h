#ifndef PROJECT_DFSSYMBOLICEXECUTION_H
#define PROJECT_DFSSYMBOLICEXECUTION_H

#include <unordered_map>
#include <stack>

#include "../CFGOpt/CFG.h"
#include "SymbolicVariables.h"
#include "SymbolicStack.h"
#include "SymbolicVarSet.h"
#include "../compile/Token.h" //relop

typedef std::unique_ptr<SymbolicVariable> VarPointer;
template <typename T>
using VarTemplatePointer = std::shared_ptr<SymbolicVariableTemplate<T>>;

namespace SymbolicExecution
{
    struct Condition
    {
        std::string l;
        Relations::Relop c;
        std::string r;

        Condition(std::string& lhs, Relations::Relop comp, std::string& rhs):
                l(lhs), c(comp), r(rhs) {}
        Condition(std::string& lhs, Relations::Relop comp, double rhs):
                l(lhs), c(comp), r(std::to_string(rhs)) {}
        Condition() = default;

        std::string toString()
        {
            return l + relEnumStrs[c] + r;
        }
    };

    struct SymbolicExecutionFringe
    {
        bool feasable = true;
        std::shared_ptr<SymbolicExecutionFringe> parent;
        std::unordered_map<std::string, Condition> pathConditions;
        std::vector<std::string> visitOrder;

        SymbolicExecutionFringe(Reporter& r);
        SymbolicExecutionFringe(std::shared_ptr<SymbolicExecutionFringe> p);

        std::shared_ptr<SymbolicStack> currentStack;
        std::shared_ptr<SymbolicVarSet> symbolicVarSet;
        Reporter& reporter;

        std::string printPathConditions();
        void addPathCondition(const std::string& nodeName, JumpOnComparisonCommand* jocc, bool negate = false);
        void error(Reporter::AlertType a, std::string s, int linenum = -1);
        void warn(Reporter::AlertType a, std::string s, int linenum = -1);
        bool isFeasable();
        bool hasSeen(std::string state);
    };

    class SymbolicExecutionManager
    {
    private:
        std::unordered_map<std::string, int> feasableVisits;
        std::unordered_map<std::string, std::shared_ptr<SymbolicVarSet>> tags;

        ControlFlowGraph& cfg;
        SymbolTable& sTable;
        Reporter& reporter;

        //returns if a feasable path extention goes through this node
        //a path is feasable if it visits itself or reaches the last state
        bool visitNode(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n);
        //the below don't check if the ranges are disjoint - this is done in visitNode
        bool branch(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n, std::string lhsvar,
                    Relations::Relop op, const std::string& rhsconst, bool reverse = false);
        
        bool branchEQ(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      std::string lhsvar, const std::string& rhsconst, bool reverse = false);
        
        bool branchNE(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      std::string lhsvar, const std::string& rhsconst, bool reverse = false);
        
        bool branchLE(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      std::string lhsvar, const std::string& rhsconst, bool reverse = false);
        
        bool branchLT(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      std::string lhsvar, const std::string& rhsconst, bool reverse = false);
        
        bool branchGE(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                           std::string lhsvar, const std::string& rhsconst, bool reverse = false);
        
        bool branchGT(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      std::string lhsvar, const std::string& rhsconst, bool reverse = false);
        
        bool varBranch(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                       SymbolicVariable* lhsvar, Relations::Relop op, SymbolicVariable* rhsvar);
        
        bool varBranchEQ(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      SymbolicVariable* lhsvar, SymbolicVariable* rhsvar);
        
        bool varBranchNE(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      SymbolicVariable* lhsvar, SymbolicVariable* rhsvar);
        
        bool varBranchLE(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      SymbolicVariable* lhsvar, SymbolicVariable* rhsvar);
        
        bool varBranchLT(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      SymbolicVariable* lhsvar, SymbolicVariable* rhsvar);
        
        bool varBranchGE(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      SymbolicVariable* lhsvar, SymbolicVariable* rhsvar);
        
        bool varBranchGT(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      SymbolicVariable* lhsvar, SymbolicVariable* rhsvar);
        SymbolicVariable* getGreatestLowerBound(SymbolicVariable* lhsvar, SymbolicVariable* rhsvar);
        SymbolicVariable* getLeastUpperBound(SymbolicVariable* lhsvar, SymbolicVariable* rhsvar);

        static CFGNode* getFailNode(std::shared_ptr<SymbolicExecutionFringe> returningSEF, CFGNode* n);

    public:
        SymbolicExecutionManager(ControlFlowGraph& cfg, SymbolTable& sTable, Reporter& reporter):
            cfg(cfg), sTable(sTable), reporter(reporter)
        {
            for (const auto& pair : cfg.getCurrentNodes()) feasableVisits[pair.first] = 0;
        };
        std::unordered_map<std::string, std::shared_ptr<SymbolicVarSet>>& search();
    };
}

#endif