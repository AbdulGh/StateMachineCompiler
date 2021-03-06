#ifndef PROJECT_DATAFLOW_H
#define PROJECT_DATAFLOW_H

#include <set>
#include <unordered_map>
#include <stack>
#include <algorithm>

#include "CFG.h"
#include "../compile/Functions.h"

namespace DataFlow
{
    std::vector<CFGNode*> getSuccessorNodes(CFGNode* node);
    std::vector<CFGNode*> getPredecessorNodes(CFGNode* node);


    //as Ts are put in ordered sets they must have comparison stuff
    template<typename T>
    std::set<T> intersectSets(std::vector<std::set<T>*>& sets)
    {
        auto setIterator = sets.begin();
        std::set<T> intersect = **setIterator;
        ++setIterator;

        //debug
        while (!intersect.empty() && setIterator != sets.end())
        {
            std::set<T>* secondPtr = *setIterator;
            for (auto it = intersect.begin(); it != intersect.end();)
            {
                if (secondPtr->find(*it) == secondPtr->end())
                {
                    it = intersect.erase(it);
                }
                else ++it;
            }
            ++setIterator;
        }
        return intersect;

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
    std::set<T> intersectPredecessors(CFGNode* node, std::unordered_map<std::string, std::set<T>>& outSets)
    {
        std::vector <std::set<T>*> predSets; //by reference!

        for (const auto& pair : node->getPredecessorMap())
        {
            std::set<T>& parentOut = outSets[pair.first];
            if (!parentOut.empty()) predSets.push_back(&parentOut);
            else return std::set<T>();
        }

        if (predSets.empty()) return std::set<T>();
        else return move(intersectSets(predSets));
    }

    template<typename T>
    std::set<T> unionPredecessors(CFGNode* node, std::unordered_map <std::string, std::set<T>>& outSets)
    {
        std::set<T> join;
        for (const auto& pair : node->getPredecessorMap())
        {
            for (const T& goingIn: outSets[pair.first]) join.insert(goingIn);
        }
        return join;
    }

    template<typename T>
    std::vector<std::set<T>*> getSuccessorOutSets(CFGNode* node, std::unordered_map<std::string, std::set<T>>& outSets)
    {
        std::vector <std::set<T>*> succSets;
        for (const auto& successor : getSuccessorNodes(node))
        {
            if (!outSets[successor->getName()].empty()) succSets.push_back(&outSets[successor->getName()]);
        }
        return succSets;
    }

    template<typename T>
    std::set<T> intersectSuccessors(CFGNode* node, std::unordered_map <std::string, std::set<T>>& outSets)
    {
        std::vector <std::set<T>*> succSets = getSuccessorOutSets(node, outSets);
        if (succSets.empty()) return std::set<T>(); //return empty std::set
        else return intersectSets(succSets);
    }

    template<typename T>
    std::set<T> unionSuccessors(CFGNode* node, std::unordered_map<std::string, std::set<T>>& outSets)
    {
        std::set<T> join;
        for (const auto& outSet : getSuccessorOutSets(node, outSets))
        {
            for (const T& goingIn: *outSet) join.insert(goingIn);
        }
        return join;
    }

    template<typename T,
            std::set<T>(*in)(CFGNode* , std::unordered_map<std::string, std::set<T>>&),
            std::vector<CFGNode*> (*nextNodes) (CFGNode*)>
    class AbstractDataFlow
    {
    protected:
        std::unordered_map<std::string, std::set<T>> outSets;
        std::vector<CFGNode*> nodes;
        ControlFlowGraph& cfg;
        SymbolTable& symbolTable;
        bool checkNodes; //used if we are only doing data flow on a part of the graph
    public:
        AbstractDataFlow(ControlFlowGraph& controlFlowGraph, SymbolTable& st)
                :symbolTable(st), cfg(controlFlowGraph), checkNodes(false)
        {
            for (const auto& pair : cfg.getCurrentNodes()) nodes.push_back(pair.second.get());
        };

        AbstractDataFlow(std::vector<CFGNode*> nodeList, ControlFlowGraph& controlFlowGraph, SymbolTable& st)
                :symbolTable(st), checkNodes(true), nodes(move(nodeList)), cfg(controlFlowGraph) {};

        void worklist()
        {
            std::stack<CFGNode*, std::vector<CFGNode*>> list(nodes);

            while (!list.empty())
            {
                CFGNode* top = list.top();
                list.pop();
                std::set<T> inSet = in(top, outSets);
                transfer(inSet, top);
                if (outSets[top->getName()] != inSet) //inSet has been transferred to new outset
                {
                    outSets[top->getName()] = move(inSet);
                    for (CFGNode* nextNode : nextNodes(top)) list.push(nextNode);
                }
            }
            finish();
        }

        virtual void transfer(std::set<T>& inSet, CFGNode* node) = 0;
        virtual void finish() = 0;
    };

    struct Assignment
    {
        std::string lhs;
        Atom rhs;

        Assignment(std::string l, Atom r) : lhs(move(l)), rhs(std::move(r))
        {}

        bool operator<(const Assignment& right) const
        {
            return (lhs < right.lhs) || (lhs == right.lhs && rhs < right.rhs);
        }

        bool operator==(const Assignment& other) const
        {
            return lhs == other.lhs && rhs == other.rhs;
        }
    };

    class AssignmentPropogationDataFlow : public AbstractDataFlow<DataFlow::Assignment,
                                                                  DataFlow::intersectPredecessors<DataFlow::Assignment>,
                                                                  DataFlow::getSuccessorNodes>
    {
    private:
        std::unordered_map<std::string, std::set<Assignment>> genSets;
        std::unordered_map<std::string, std::set<std::string>> killSets;
    public:
        AssignmentPropogationDataFlow(ControlFlowGraph& cfg, SymbolTable& st);

        void transfer(std::set<Assignment>& in, CFGNode* node) override;
        void finish() override;
    };

    class LiveVariableDataFlow : public AbstractDataFlow<std::string,
                                                         DataFlow::unionSuccessors<std::string>,
                                                         DataFlow::getPredecessorNodes>
    {
    private:
        std::set<std::string> usedVars;
        std::unordered_map<std::string, std::set<std::string>> UEVars;
        std::unordered_map<std::string, std::set<std::string>> genSets;
        std::unordered_map<std::string, std::set<std::string>> killSets;
    public:
        LiveVariableDataFlow(ControlFlowGraph& cfg, SymbolTable& st);

        void transfer(std::set<std::string>& in, CFGNode* node) override;

        void finish() override;
    };
}

#endif