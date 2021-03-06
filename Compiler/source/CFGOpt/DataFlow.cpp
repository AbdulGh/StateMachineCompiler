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
                case CommandType::NONDET:
                {
                    auto nc = static_cast<NondetCommand*>(instr.get());
                    if (!nc->holding) continue;
                } //meant to fall through here
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
                    genSet.insert(Assignment(lhs, Atom(0)));
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
        auto it = find_if(in.begin(), in.end(), [&, kill] (const Assignment& ass) {return ass.lhs == kill;});
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
        //node->constProp(move(mapToPass));
    }
}

//LiveVariableDataFlow

LiveVariableDataFlow::LiveVariableDataFlow(ControlFlowGraph& cfg, SymbolTable& st)
        : AbstractDataFlow(cfg, st)
{
    for (CFGNode* node : nodes) //intra-propogation already done in Optimiser
    {
        set<string> thisUEVars;
        set<string> genSet;
        set<string> killSet;

        auto insertAndCheckUpwardExposed = [&genSet, &thisUEVars, &killSet, this](const VarWrapper* inserting) -> void
        {
            if (inserting->isCompound())
            {
                for (const std::string* namePtr : inserting->getAllNames())
                {
                    const std::string& name = *namePtr;
                    if (!isdigit(name[0]) && name[0] != '"')
                    {
                        auto it = killSet.find(name);
                        if (it == killSet.end()) thisUEVars.insert(name);
                        genSet.insert(name);
                        usedVars.insert(name);
                    }
                }
            }

            else
            {
                const std::string& name = inserting->getBaseName();
                if (!isdigit(name[0]) && name[0] != '"')
                {
                    auto it = killSet.find(name);
                    if (it == killSet.end()) thisUEVars.insert(name);
                    genSet.insert(name);
                    usedVars.insert(name);
                }
            }
        };

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
                    usedVars.insert(instr->getVarWrapper()->getBaseName());
                    genSet.insert(instr->getVarWrapper()->getBaseName());
                    killSet.insert(instr->getVarWrapper()->getBaseName());
                    break;
                }
                case CommandType::NONDET:
                {
                    auto nc = static_cast<NondetCommand*>(instr.get());
                    const string& bname = (nc->holding) ? nc->getVarWrapper()->getBaseName() : nc->getString();
                    usedVars.insert(bname);
                    genSet.insert(bname);
                    killSet.insert(bname);
                    break;
                }
                
                //simple commands that just read some variable
                case CommandType::PUSH:
                {
                    PushCommand* pc = static_cast<PushCommand*>(instr.get());
                    if (pc->pushesState() || !pc->getAtom().isHolding()) continue;
                    else killSet.insert(pc->getAtom().getVarWrapper()->getBaseName());

                }
                case CommandType::PRINT:
                {
                    const Atom& at = instr->getAtom();
                    if (at.getType() == StringType::ID) insertAndCheckUpwardExposed(at.getVarWrapper());
                    break;
                }
                case CommandType::ASSIGNVAR:
                {
                    killSet.insert(instr->getVarWrapper()->getBaseName());
                    const Atom& rhs = instr->getAtom();
                    if (rhs.getType() == StringType::ID) insertAndCheckUpwardExposed(rhs.getVarWrapper());
                    break;
                }
                case CommandType::EXPR:
                {
                    EvaluateExprCommand* eec = static_cast<EvaluateExprCommand*>(instr.get());
                    killSet.insert(eec->getVarWrapper()->getBaseName());
                    if (eec->term1.isHolding()) insertAndCheckUpwardExposed(eec->term1.getVarWrapper());
                    if (eec->term2.isHolding()) insertAndCheckUpwardExposed(eec->term2.getVarWrapper());
                }
            }
        }

        JumpOnComparisonCommand* jocc = node->getComp();
        if (jocc != nullptr)
        {
            if (jocc->term1.getType() == StringType::ID)
            {
                insertAndCheckUpwardExposed(jocc->term1.getVarWrapper());
            }
            if (jocc->term2.getType() == StringType::ID)
            {
                insertAndCheckUpwardExposed(jocc->term2.getVarWrapper());
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

            switch (ac->getType())
            {
                case CommandType::NONDET:
                {
                    auto ndc = static_cast<NondetCommand*>(ac.get());
                    if (ndc->holding) name = ac->getVarWrapper()->getBaseName();
                    else name = ndc->getString();
                    break;
                }
                case CommandType::ASSIGNVAR:
                case CommandType::EXPR:
                case CommandType::INPUTVAR:
                {
                    name = ac->getVarWrapper()->getBaseName();
                    break;
                }

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
                if (ac->getType() == CommandType::DECLAREVAR && usedVars.find(name) != usedVars.end()) //look past this bit
                {
                    DeclareCommand* dvc = static_cast<DeclareCommand*>(ac.get());
                    if (dvc->dt == DeclareCommand::DeclareType::ARRAY)
                    {
                        DeclareArrayCommand* dvc = static_cast<DeclareArrayCommand*>(ac.get());
                        //todo next toDeclare stuff for arrays

                    }
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
    for (auto& var: toDeclare) startNode->appendDeclatation(var.second);
}