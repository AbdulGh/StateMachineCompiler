#include <algorithm>

#include "DataFlow.h"
#include "../symbolic/VarWrappers.h"

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
        : AbstractDataFlow(cfg, st)
{
    for (CFGNode* node : nodes) //intra-propogation already done in Optimiser
    {
        set<Assignment> genSet;
        set<string> killSet;

        vector<unique_ptr<AbstractCommand>>& instrs = node->getInstrs();
        
        for (const auto& instr : instrs)
        {
            switch (instr->getType())
            {
                case CommandType::INPUTVAR:
                case CommandType::EXPR:
                case CommandType::POP:
                {
                    const std::string& data = instr->getVarWrapper()->getBaseName();
                    auto it = find_if(genSet.begin(), genSet.end(),
                                      [&, data](const Assignment& ass)
                                      { return ass.lhs == data; });
                    if (it != genSet.end()) genSet.erase(it);
                    killSet.insert(data);
                    break;
                }
                case CommandType::ASSIGNVAR:
                {
                    const string& lhs = instr->getVarWrapper()->getFullName();
                    auto it = find_if(genSet.begin(), genSet.end(),
                                      [&, lhs](const Assignment& ass)
                                      { return ass.lhs == lhs; });
                    if (it != genSet.end()) genSet.erase(it);
                    genSet.insert(Assignment(lhs, instr->getAtom()));
                    killSet.erase(lhs);
                    break;
                }
                case CommandType::DECLAREVAR:
                {
                    DeclareCommand* dvc = static_cast<DeclareCommand*>(instr.get());
                    if (dvc->dt == DeclareCommand::DeclareType::ARRAY) continue;

                    const string& lhs = dvc->getBaseName();
                    auto it = find_if(genSet.begin(), genSet.end(),
                                      [&, lhs](const Assignment& ass)
                                      { return ass.lhs == lhs; });
                    if (it != genSet.end()) genSet.erase(it);
                    string defaultStr = dvc->vt == DOUBLE ? "0" : "";
                    genSet.insert(Assignment(lhs, Atom(defaultStr, true)));
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
    for (CFGNode* node : nodes)
    {
        set<Assignment> inAss = intersectPredecessors(node, outSets);

        unordered_map<string, Atom> mapToPass;
        for (const Assignment& ass : inAss) mapToPass.emplace(ass.lhs, ass.rhs);
        node->constProp(move(mapToPass));
    }
}

//LiveVariableDataFlow

#define insertAndCheckUpwardExposed(inserting) \
    if (!isdigit(inserting[0]) && inserting[0] != '"')\
    {\
        it = killSet.find(inserting);\
        if (it == killSet.end()) thisUEVars.insert(inserting);\
        genSet.insert(inserting);\
        usedVars.insert(inserting);\
    }

LiveVariableDataFlow::LiveVariableDataFlow(ControlFlowGraph& cfg, SymbolTable& st)
        : AbstractDataFlow(cfg, st)
{
    for (CFGNode* node : nodes) //intra-propogation already done in Optimiser
    {
        set<string> thisUEVars;
        set<string> genSet;
        set<string> killSet;
        set<string>::iterator it;

        vector<unique_ptr<AbstractCommand>>& instrs = node->getInstrs();

        for (const auto& instr : instrs)
        {
            switch (instr->getType())
            {
                //commands that just stop some variable being live
                case CommandType::DECLAREVAR:
                {
                    DeclareCommand* dc = static_cast<DeclareCommand*>(instr.get());
                    killSet.insert(dc->getBaseName());
                    break;
                }
                case CommandType::INPUTVAR:
                case CommandType::POP:
                {
                    killSet.insert(instr->getVarWrapper()->getBaseName());
                    break;
                }
                //simple commands that just read some variable
                case CommandType::PUSH:
                {
                    PushCommand* pc = static_cast<PushCommand*>(instr.get());
                    if (pc->pushesState()) continue;
                }
                case CommandType::PRINT:
                {
                    const Atom& at = instr->getAtom();
                    if (at.getType() == StringType::ID) insertAndCheckUpwardExposed(at.getVarWrapper()->getBaseName());
                    break;
                }
                case CommandType::ASSIGNVAR:
                {
                    killSet.insert(instr->getVarWrapper()->getBaseName());
                    const Atom& rhs = instr->getAtom();
                    if (rhs.getType() == StringType::ID) insertAndCheckUpwardExposed(rhs.getVarWrapper()->getBaseName());
                    break;
                }
                case CommandType::EXPR:
                {
                    EvaluateExprCommand* eec = static_cast<EvaluateExprCommand*>(instr.get());
                    killSet.insert(eec->getVarWrapper()->getBaseName());
                    if (!eec->term1.isLit) insertAndCheckUpwardExposed(eec->term1.vg->getBaseName());
                    if (!eec->term2.isLit) insertAndCheckUpwardExposed(eec->term2.vg->getBaseName());
                }
            }
        }

        JumpOnComparisonCommand* jocc = node->getComp();
        if (jocc != nullptr)
        {
            if (jocc->term1.getType() == StringType::ID)
            {
                insertAndCheckUpwardExposed(jocc->term1.getVarWrapper()->getBaseName());
            }
            if (jocc->term2.getType() == StringType::ID)
            {
                insertAndCheckUpwardExposed(jocc->term2.getVarWrapper()->getBaseName());
            }
        }

        outSets[node->getName()] = thisUEVars;//copies
        UEVars[node->getName()] = move(thisUEVars);
        genSets[node->getName()] = move(genSet);
        killSets[node->getName()] = move(killSet);
    }
}

void LiveVariableDataFlow::transfer(set<string>& in, CFGNode* node)
{
    for (auto& exposed: UEVars[node->getName()]) in.insert(exposed);
}

void LiveVariableDataFlow::finish()
{
    set<pair<VariableType,string>> toDeclare;
    for (CFGNode* node : nodes)
    {
        set<string>& liveOut = outSets[node->getName()];
        set<string>& genSet = genSets[node->getName()];
        vector<unique_ptr<AbstractCommand>> newInstrs;

        //we remove commands that assign stuff or declare dead vars
        auto isDead = [&, liveOut](const string& varN) -> bool
        {
            return liveOut.find(varN) == liveOut.end() && genSet.find(varN) == genSet.end();
        };

        for (auto& ac : node->getInstrs())
        {
            std::string name;
            CommandType acType = ac->getType();

            switch (ac->getType())
            {
                case CommandType::ASSIGNVAR:
                case CommandType::EXPR:
                case CommandType::INPUTVAR:
                    name = ac->getVarWrapper()->getBaseName();
                    break;

                case CommandType::DECLAREVAR:
                {
                    DeclareCommand *dvc = static_cast<DeclareVarCommand *>(ac.get());
                    name = dvc->getBaseName();
                    break;
                }
                default:
                    newInstrs.push_back(move(ac));
                    continue;
            }
            
            if (isDead(name))
            {
                if (ac->getType() == CommandType::DECLAREVAR && usedVars.find(name) != usedVars.end())
                {
                    DeclareVarCommand* dvc = static_cast<DeclareVarCommand*>(ac.get());
                    VariableType vt = dvc->dt == DeclareCommand::DeclareType::ARRAY ? VariableType::ARRAY : dvc->vt;
                    toDeclare.insert({vt, name});
                }
            }

            else
            {
                if (ac->getType() == CommandType::POP && isDead(ac->getVarWrapper()->getBaseName()))
                {
                    PopCommand* pc = static_cast<PopCommand*>(ac.get());
                    pc->clear();
                }
                newInstrs.push_back(move(ac));
            }
        }

        node->setInstructions(newInstrs);
    }

    CFGNode* startNode = cfg.getFirst();
    for (auto& var: toDeclare) startNode->appendDeclatation(var.first, var.second);
}