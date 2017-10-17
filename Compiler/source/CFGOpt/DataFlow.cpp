#include <unordered_map>
#include <stack>
#include <algorithm>

#include "DataFlow.h"

#define instantiateTemplateFunctions(type) \
    template <typename T> set<type> intersectSets<type>(vector<set<type>&>& sets);\
    template <typename T> set<type> intersectPredecessors<type>(CFGNode* node, unordered_map<string, set<type>>& outSets);\
    template <typename T> set<type> unionPredecessors<type>(CFGNode* node, unordered_map<string, set<type>>& outSets);\
    template <typename T> set<type> intersectSuccessors<type>(CFGNode* node, unordered_map<string, set<type>>& outSets);\
    template <typename T> set<type> unionSuccessors<type>(CFGNode* node, unordered_map<string, set<type>>& outSets);\

using namespace std;
using namespace DataFlow;

template <typename T, set<T>(*in)(CFGNode*, unordered_map<string, set<T>>&)>
void AbstractDataFlow<T, in>::worklist()
{
	unordered_map<string, set<T>> outSets;
	stack<CFGNode*> list({controlFlowGraph.getFirst()});
	
	while (!list.empty())
	{
		CFGNode* top = list.top();
        list.pop();
        set<T> inSet = in(top, outSets);
		transfer(inSet, top);
		if (outSets[top->getName()].size() != inSet.size())
		{
			outSets[top->getName()] = move(inSet);
			if (top->getCompFail() != nullptr) list.push(top->getCompFail());
			if (top->getCompSuccess() != nullptr) list.push(top->getCompSuccess());
			if (top->isLastNode()) for (CFGNode* n : top->getParentFunction()->getReturnSuccessors()) list.push(n);
		}
	}
    finish();
}
template class AbstractDataFlow<Assignment, &intersectPredecessors<Assignment>>;

//AssignmentPropogationDataFlow

AssignmentPropogationDataFlow::AssignmentPropogationDataFlow(ControlFlowGraph& cfg, SymbolTable& st)
        : AbstractDataFlow(cfg, st)
{
    for (const auto& pair : controlFlowGraph.getCurrentNodes()) //intra-propogation already done in Optimiser
    {
        CFGNode* node = pair.second.get();
        set<Assignment> genSet;
        set<string> killSet;

        //these lambdas should be inlined
        auto findAssignment = [&, genSet](const string& name) -> set<Assignment>::iterator
        {
            return find_if(genSet.begin(), genSet.end(),
                           [name] (const Assignment& ass) {return ass.lhs == name;});
        };

        /*auto eraseAssignment = [&, genSet]( string& name, bool addToKillSet) -> void
        {
            auto it = findAssignment(name);
            if (it != genSet.end()) genSet.erase(it);
            if (addToKillSet) killSet.insert(name);
        };

        auto insertAssignment = [&, genSet](const string& lhs, const string& rhs) -> void
        {

        };*/

        vector<unique_ptr<AbstractCommand>>& instrs = node->getInstrs();
        
        for (const auto& instr : instrs)
        {
            if (instr->getType() == CommandType::CHANGEVAR)
            {
                auto it = findAssignment(node->getName());
                if (it != genSet.end()) genSet.erase(it);
                killSet.insert(node->getName());
            }
            else if (instr->getType() == CommandType::ASSIGNVAR)
            {
                auto avc = static_cast<AssignVarCommand*>(instr.get());
                const string& lhs = avc->getData();
                auto it = findAssignment(lhs);
                if (it != genSet.end()) genSet.erase(it);
                genSet.insert(Assignment(lhs, avc->RHS));
                killSet.erase(lhs);
            }
        }

        genSets[node->getName()] = move(genSet);
        killSets[node->getName()] = move(killSet);
    }
}

void AssignmentPropogationDataFlow::transfer(set<Assignment>& in, CFGNode* node)
{
    for (auto& ass: genSets[node->getName()]) in.insert(ass);
    for (auto& kill : killSets[node->getName()])
    {
        auto it = find_if(in.begin(), in.end(), [kill] (const Assignment& ass) {return ass.lhs == kill;});
        if (it != in.end()) in.erase(it);
    }
}

void AssignmentPropogationDataFlow::finish()
{
    for (auto& pair : controlFlowGraph.getCurrentNodes())
    {
        unique_ptr<CFGNode>& node = pair.second;
        set<Assignment> inAss = intersectPredecessors(node.get(), outSets);

        unordered_map<string, string> mapToPass;
        for (const Assignment& ass : inAss) mapToPass[ass.lhs] = ass.rhs;
        node->constProp(move(mapToPass));
    }
}