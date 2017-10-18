#include <set>

#include "CFG.h"
#include "../compile/Functions.h"

namespace DataFlow
{
    //as Ts are put in ordered sets they must have comparison stuff
    template<typename T>
    std::set<T> intersectSets(std::vector<std::set<T>*>& sets)
    {
        auto setIterator = sets.begin();
        std::set<T> intersect = **setIterator;
        ++setIterator;
        //sets are sorted so this works and is reasonably fast
        while (!intersect.empty() && setIterator != sets.end())
        {
            auto intersectIterator = intersect.begin();
            auto parentIter = (*setIterator)->begin();

            while (intersectIterator != intersect.end() && parentIter != (*setIterator)->end())
            {
                if (*intersectIterator < *parentIter) intersectIterator = intersect.erase(intersectIterator);
                else if (*parentIter < *intersectIterator) ++parentIter;
                else
                {
                    ++intersectIterator;
                    ++parentIter;
                }
            }
            ++setIterator;
        }
        return intersect;
    }

    template<typename T>
    std::set<T> intersectPredecessors(CFGNode *node, std::unordered_map<std::string, std::set<T>>& outSets)
    {
        std::vector < std::set<T>* > predSets; //by reference!
        for (const auto& pair : node->getPredecessors())
        {
            std::set<T>& parentOut = outSets[pair.first];
            if (!parentOut.empty()) predSets.push_back(&parentOut);
        }

        if (node->getName() == "F0_main_5")
        {
            int size = predSets.size();
            std::vector<std::set<T>> predies;
            for (const auto& pair : node->getPredecessors()) predies.push_back(outSets[pair.first]);
            //auto wilkins = (*(predSets.at(0)->begin()));
            int debug;
            debug = '3';
        }

        if (predSets.empty()) return std::set<T>(); //return set
        else return move(intersectSets(predSets));
    }

    template<typename T>
    std::set<T> unionPredecessors(CFGNode *node, std::unordered_map <std::string, std::set<T>>& outSets)
    {
        std::set<T> join;
        for (const auto& pair : node->getPredecessors())
        {
            for (const T& goingIn: outSets[pair.first]) join.insert(goingIn);
        }
        return join;
    }

    template<typename T>
    std::vector<std::set<T>*> getSuccessorOutSets(CFGNode *node, std::unordered_map<std::string, std::set<T>>& outSets)
    {
        std::vector <std::set<T>*> succSets;
        if (node->getCompSuccess() != nullptr)
        {
            std::set<T>& compSuccessOutSet = outSets[node->getCompSuccess()->getName()];
            if (!compSuccessOutSet.empty()) succSets.push_back(&compSuccessOutSet);
        }
        if (node->getCompFail() != nullptr)
        {
            std::set<T>& compFailOutSet = outSets[node->getCompFail()->getName()];
            if (!compFailOutSet.empty()) succSets.push_back(&compFailOutSet);
        } else
        {
            if (!node->isLastNode()) throw "only last node can return";
            for (const auto& retSucc : node->getParentFunction()->getReturnSuccessors())
            {
                std::set<T>& returnSuccessorOutSet = outSets[retSucc->getName()];
                if (!returnSuccessorOutSet.empty()) succSets.push_back(&returnSuccessorOutSet);
            }
        }
        return succSets;
    }

    template<typename T>
    std::set<T> intersectSuccessors(CFGNode *node, std::unordered_map <std::string, std::set<T>>& outSets)
    {
        std::vector <std::set<T>*> succSets = getSuccessorOutSets(node, outSets);
        if (succSets.empty()) return std::set<T>(); //return empty std::set
        else return intersectSets(succSets);
    }

    template<typename T>
    std::set<T> unionSuccessors(CFGNode *node, std::unordered_map<std::string, std::set<T>>& outSets)
    {
        std::set<T> join;
        for (const auto& pair : getSuccessorOutSets(node, outSets))
        {
            for (const T& goingIn: outSets[pair.first]) join.insert(goingIn);
        }
        return join;
    }

    template<typename T, std::set<T>(*in)(CFGNode *, std::unordered_map<std::string, std::set<T>>&)>
    class AbstractDataFlow
    {
    protected:
        std::unordered_map<std::string, std::set<T>> outSets;
        ControlFlowGraph& controlFlowGraph;
        SymbolTable& symbolTable;
    public:
        AbstractDataFlow(ControlFlowGraph& cfg, SymbolTable& st) : controlFlowGraph(cfg), symbolTable(st)
        {};

        void worklist();

        virtual void transfer(std::set<T>& inSet, CFGNode *node) = 0; //must be monotone
        virtual void finish() = 0;
    };

    struct Assignment
    {
        std::string lhs;
        std::string rhs;

        Assignment(std::string l, std::string r) : lhs(move(l)), rhs(move(r))
        {}

        bool operator<(const Assignment& right) const
        {
            return (lhs < right.lhs) || (lhs == right.lhs && rhs < right.rhs);
        }
    };

    class AssignmentPropogationDataFlow : public AbstractDataFlow<Assignment, intersectPredecessors<Assignment>>
    {
    private:
        std::unordered_map<std::string, std::set<Assignment>> genSets;
        std::unordered_map<std::string, std::set<std::string>> killSets;
    public:
        AssignmentPropogationDataFlow(ControlFlowGraph& cfg, SymbolTable& st);

        void transfer(std::set<Assignment>& in, CFGNode *node) override;

        void finish() override;
    };
}
