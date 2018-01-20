#ifndef PROJECT_DFSSYMBOLICEXECUTION_H
#define PROJECT_DFSSYMBOLICEXECUTION_H

#include <unordered_map>
#include <stack>

#include "../CFGOpt/CFG.h"
#include "SymbolicVariables.h"
#include "SymbolicStack.h"
#include "SymbolicVarSet.h"
#include "../compile/Token.h" //relop
#include "../compile/Functions.h"

//todo improve expressions in first execution (ruin determination only one way)

namespace SymbolicExecution
{
    struct Condition
    {
        std::string lhs;
        Relations::Relop comp;
        std::string rhs;

        Condition(std::string& lhs, Relations::Relop comp, std::string& rhs):
                lhs(lhs), comp(comp), rhs(rhs) {}
        Condition(std::string& lhs, Relations::Relop comp, double rhs):
                lhs(lhs), comp(comp), rhs(std::to_string(rhs)) {}
        Condition() = default;

        std::string toString()
        {
            return lhs + relEnumStrs[comp] + rhs;
        }
    };

    struct SymbolicExecutionFringe
    {
        bool feasable = true;
        bool checkParentPC = true;
        std::shared_ptr<SymbolicExecutionFringe> parent;
        std::unordered_map<std::string, Condition> pathConditions;
        std::vector<std::string> seenFunctionCalls;
        std::vector<std::string> visitOrder;

        SymbolicExecutionFringe() = delete;
        explicit SymbolicExecutionFringe(Reporter& r);
        explicit SymbolicExecutionFringe(std::shared_ptr<SymbolicExecutionFringe> p);

        std::shared_ptr<SymbolicStack> symbolicStack;
        std::shared_ptr<SymbolicVarSet> symbolicVarSet;
        Reporter& reporter;

        std::string printPathConditions();
        bool addPathCondition(const std::string& nodeName, JumpOnComparisonCommand* jocc, bool negate = false);
        void setLoopInit();
        void error(Reporter::AlertType a, std::string s, int linenum = -1);
        void warn(Reporter::AlertType a, std::string s, int linenum = -1);
        bool isFeasable();
        bool hasSeen(const std::string& state);
        std::vector<Condition> getConditions();
    };

    class SymbolicExecutionManager
    {
    public:
        class SearchResult
        {
        private:
            unsigned int poppedCounter = 0;
            std::vector<SymVarStackMember> pseudoStack; //todo replace this
            SymbolicStack symbolicStack;
            std::shared_ptr<SymbolicVarSet> svs;

        public:
            explicit SearchResult(Reporter& r) : symbolicStack(r, true)
            {
                svs = std::make_shared<SymbolicVarSet>(nullptr);
            };

            std::shared_ptr<SymbolicVarSet> getInitSVS()
            {return std::make_shared<SymbolicVarSet>(svs);};

            bool unionSVS(SymbolicVarSet* other)
            {
                return svs->unionSVS(other);
            }
            bool unionStack(SymbolicStack* other)
            {
                return symbolicStack.assimilateChanges(other);
            }

            void resetPoppedCounter() {poppedCounter = 0;}
            void incrementPoppedCounter()
            {
                if (poppedCounter >= pseudoStack.size()) throw "went too far";
                ++poppedCounter;
            }

            void addPop(std::unique_ptr<SymbolicVariable> sv)
            {
                if (poppedCounter == pseudoStack.size()) pseudoStack.emplace_back(SymVarStackMember(move(sv)));
                else
                {
                    SymbolicDouble sd("constDouble", sv->getReporter());
                    pseudoStack[poppedCounter].mergeVar(*sv);
                }
                ++poppedCounter;
            }

            inline bool hasPop() const {return poppedCounter < pseudoStack.size();}

            std::unique_ptr<SymbolicVariable> popVar()
            {
                if (!hasPop()) throw "popped empty stack";
                return pseudoStack[poppedCounter++].varptr->clone();
            }
        };

        SymbolicExecutionManager(ControlFlowGraph& cfg, SymbolTable& sTable, Reporter& reporter):
                cfg(cfg), sTable(sTable), reporter(reporter)
        {};
        std::unordered_map<std::string, std::unique_ptr<SearchResult>>& search();

    private:
        std::set<std::string> visitedNodes;
        std::unordered_map<std::string, std::unique_ptr<SearchResult>> tags;
        //std::unordered_map<std::string, std::set<std::string>> seenReturnStates;

        ControlFlowGraph& cfg;
        SymbolTable& sTable;
        Reporter& reporter;

        //returns if a feasable path extention goes through this node
        //a path is feasable if it visits itself or reaches the last state
        void visitNode(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n);
        //the below don't check if the ranges are disjoint - this is done in visitNode
        void branch(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n, std::string lhsvar,
                    Relations::Relop op, const std::string& rhsconst, bool reverse = false);
        
        void branchEQ(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      std::string lhsvar, const std::string& rhsconst, bool reverse = false);
        
        void branchNE(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      std::string lhsvar, const std::string& rhsconst, bool reverse = false);
        
        void branchLE(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      std::string lhsvar, const std::string& rhsconst, bool reverse = false);
        
        void branchLT(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      std::string lhsvar, const std::string& rhsconst, bool reverse = false);
        
        void branchGE(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                           std::string lhsvar, const std::string& rhsconst, bool reverse = false);
        
        void branchGT(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      std::string lhsvar, const std::string& rhsconst, bool reverse = false);
        
        void varBranch(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                       SymbolicVariable* lhsvar, Relations::Relop op, SymbolicVariable* rhsvar);
        
        void varBranchEQ(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      SymbolicVariable* lhsvar, SymbolicVariable* rhsvar);
        
        void varBranchNE(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      SymbolicVariable* lhsvar, SymbolicVariable* rhsvar);
        
        void varBranchLE(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      SymbolicVariable* lhsvar, SymbolicVariable* rhsvar);
        
        void varBranchLT(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      SymbolicVariable* lhsvar, SymbolicVariable* rhsvar);
        
        void varBranchGE(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      SymbolicVariable* lhsvar, SymbolicVariable* rhsvar);
        
        void varBranchGT(std::shared_ptr<SymbolicExecutionFringe> sef, CFGNode* n,
                      SymbolicVariable* lhsvar, SymbolicVariable* rhsvar);

        static CFGNode* getFailNode(std::shared_ptr<SymbolicExecutionFringe> returningSEF, CFGNode* n);
    };
}

typedef std::unique_ptr<SymbolicExecution::SymbolicExecutionManager::SearchResult> SRPointer;

#endif