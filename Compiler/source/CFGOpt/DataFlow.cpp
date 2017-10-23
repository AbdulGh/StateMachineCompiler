#include <algorithm>

#include "DataFlow.h"

using namespace std;
using namespace DataFlow;

std::vector<CFGNode*> DataFlow::getSuccessorNodes(CFGNode* node)
{
    return node->getSuccessorVector();
}
std::vector<CFGNode*> DataFlow::getPredecessorNodes(CFGNode* node)
{
    return node->getPredecessorVector();
}

//AssignmentPropogationDataFlow
AssignmentPropogationDataFlow::AssignmentPropogationDataFlow(ControlFlowGraph& cfg, SymbolTable& st)
        : AbstractDataFlow(cfg, st, cfg.getFirst())
{
    for (const auto& pair : controlFlowGraph.getCurrentNodes()) //intra-propogation already done in Optimiser
    {
        CFGNode* node = pair.second.get();
        set<Assignment> genSet;
        set<string> killSet;

        vector<unique_ptr<AbstractCommand>>& instrs = node->getInstrs();
        
        for (const auto& instr : instrs)
        {
            switch (instr->getType())
            {
                case CommandType::CHANGEVAR:
                case CommandType::EXPR:
                {
                    const std::string& data = instr->getData();
                    auto it = find_if(genSet.begin(), genSet.end(),
                                      [&, data](const Assignment& ass)
                                      { return ass.lhs == data; });
                    if (it != genSet.end()) genSet.erase(it);
                    killSet.insert(data);
                    break;
                }
                case CommandType::ASSIGNVAR:
                {
                    auto avc = static_cast<AssignVarCommand *>(instr.get());
                    const string& lhs = avc->getData();
                    auto it = find_if(genSet.begin(), genSet.end(),
                                      [&, lhs](const Assignment& ass)
                                      { return ass.lhs == lhs; });
                    if (it != genSet.end()) genSet.erase(it);
                    genSet.insert(Assignment(lhs, avc->RHS));
                    killSet.erase(lhs);
                    break;
                }
                case CommandType::DECLAREVAR:
                {
                    DeclareVarCommand *dvc = static_cast<DeclareVarCommand *>(instr.get());
                    const string& lhs = dvc->getData();
                    auto it = find_if(genSet.begin(), genSet.end(),
                                      [&, lhs](const Assignment& ass)
                                      { return ass.lhs == lhs; });
                    if (it != genSet.end()) genSet.erase(it);
                    string defaultStr = dvc->vt == DOUBLE ? "0" : "";
                    genSet.insert(Assignment(lhs, move(defaultStr)));
                    killSet.erase(lhs);
                    break;
                }
            }
        }
        genSets[node->getName()] = move(genSet);
        killSets[node->getName()] = move(killSet);
    }
}

void AssignmentPropogationDataFlow::transfer(set<Assignment>& in, CFGNode* node)
{
    for (auto& ass: genSets[node->getName()])
    {
        auto it = find_if(in.begin(), in.end(), [&, ass](const Assignment& otherAss){return otherAss.lhs == ass.lhs;});
        if (it != in.end()) in.erase(it);
        in.insert(ass);
    }
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

#define insertIfNotInKillSet(inserting) \
    if (!isdigit(inserting[0]) && inserting[0] != '"')\
    {\
        it = killSet.find(inserting);\
        if (it == killSet.end()) genSet.insert(inserting);\
    }

//LiveVariableDataFlow
LiveVariableDataFlow::LiveVariableDataFlow(ControlFlowGraph& cfg, SymbolTable& st)
        : AbstractDataFlow(cfg, st, cfg.getLast())
{
    for (const auto& pair : controlFlowGraph.getCurrentNodes()) //intra-propogation already done in Optimiser
    {
        CFGNode* node = pair.second.get();
        set<string> genSet;
        set<string> killSet;
        set<string>::iterator it;

        vector<unique_ptr<AbstractCommand>>& instrs = node->getInstrs();

        for (const auto& instr : instrs)
        {
            switch (instr->getType())
            {
                //commands that just stop some variable being live
                case CommandType::CHANGEVAR:
                case CommandType::DECLAREVAR:
                case CommandType::POP:
                    killSet.insert(instr->getData());
                    break;
                //simple commands that just read some variable
                case CommandType::PUSH:
                {
                    PushCommand* pvc = static_cast<PushCommand*>(instr.get());
                    const string& data = pvc->getData();
                    if (pvc->pushType == PushCommand::PUSHSTR && isalnum(data[0])) insertIfNotInKillSet(instr->getData());
                    break;
                }
                case CommandType::PRINT:
                    insertIfNotInKillSet(instr->getData());
                    break;
                case CommandType::ASSIGNVAR:
                {
                    auto avc = static_cast<AssignVarCommand*>(instr.get());
                    killSet.insert(avc->getData());
                    insertIfNotInKillSet(avc->RHS);
                    break;
                }
                case CommandType::EXPR:
                {
                    EvaluateExprCommand* eec = static_cast<EvaluateExprCommand*>(instr.get());
                    killSet.insert(eec->getData());
                    insertIfNotInKillSet(eec->term1);
                    insertIfNotInKillSet(eec->term2);
                }
            }
        }

        JumpOnComparisonCommand* jocc = node->getComp();
        if (jocc != nullptr)
        {
            if (jocc->term1Type == AbstractCommand::StringType::ID) genSet.insert(jocc->term1);
            if (jocc->term2Type == AbstractCommand::StringType::ID) genSet.insert(jocc->term2);
        }
        outSets[node->getName()] = genSet;//copies
        genSets[node->getName()] = move(genSet);
    }
}

void LiveVariableDataFlow::transfer(set<string>& in, CFGNode* node)
{
    for (auto& live: genSets[node->getName()]) in.insert(live);
}

void LiveVariableDataFlow::finish()
{
    for (auto& pair : controlFlowGraph.getCurrentNodes())
    {
        unique_ptr<CFGNode>& node = pair.second;
        set<string>& liveOut = outSets[node->getName()];
        vector<unique_ptr<AbstractCommand>> newInstrs;

        //we remove commands that assign stuff or declare dead vars
        auto isDead = [&, liveOut](const string& varN) -> bool
        {
            return liveOut.find(varN) == liveOut.end();
        };
        for (auto& ac : node->getInstrs())
        {
            if ((ac->getType() == CommandType::ASSIGNVAR
                || ac->getType() == CommandType::EXPR
                || ac->getType() == CommandType::CHANGEVAR
                || ac->getType() == CommandType::DECLAREVAR) &&
                isDead(ac->getData())) continue;

            else
            {
                if (ac->getType() == CommandType::POP && isDead(ac->getData()))
                {
                    PopCommand* pc = static_cast<PopCommand*>(ac.get());
                    pc->setData("");
                }
                newInstrs.push_back(move(ac));
            }
        }
        node->setInstructions(newInstrs);
    }
}