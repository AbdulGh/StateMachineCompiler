#include <algorithm>

#include "DataFlow.h"
#include "../symbolic/SymbolicVarWrappers.h"

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
                case CommandType::CHANGEVAR:
                case CommandType::EXPR:
                case CommandType::POP:
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
                    auto avc = static_cast<AssignVarCommand*>(instr.get());
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
                    DeclareVarCommand* dvc = static_cast<DeclareVarCommand*>(instr.get());
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
    for (CFGNode* node : nodes)
    {
        set<Assignment> inAss = intersectPredecessors(node, outSets);

        unordered_map<string, string> mapToPass;
        for (const Assignment& ass : inAss) mapToPass[ass.lhs] = ass.rhs;

        node->constProp(move(mapToPass));
    }
}

//LiveVariableDataFlow
//strip array indicies
#define filterVarName(dat) dat.substr(0, dat.find('['))

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
                case CommandType::CHANGEVAR:
                case CommandType::DECLAREVAR:
                case CommandType::POP:{
                    auto debug = instr->getData();
                    killSet.insert(filterVarName(instr->getData()));
                    break;}
                //simple commands that just read some variable
                case CommandType::PUSH:
                {
                    PushCommand* pvc = static_cast<PushCommand*>(instr.get());
                    string data = filterVarName(pvc->getData());
                    if (!pvc->pushesState()
                        && AbstractCommand::getStringType(pvc->getData()) == AbstractCommand::StringType::ID)
                    {
                        insertAndCheckUpwardExposed(data);
                    }
                    break;
                }
                case CommandType::PRINT:
                    insertAndCheckUpwardExposed(filterVarName(instr->getData()));
                    break;
                case CommandType::ASSIGNVAR:
                {
                    auto avc = static_cast<AssignVarCommand*>(instr.get());
                    killSet.insert(filterVarName(avc->getData()));
                    insertAndCheckUpwardExposed(avc->RHS);
                    break;
                }
                case CommandType::EXPR:
                {
                    EvaluateExprCommand* eec = static_cast<EvaluateExprCommand*>(instr.get());
                    killSet.insert(filterVarName(eec->lhs->getBaseName()));
                    insertAndCheckUpwardExposed(filterVarName(eec->term1->getBaseName()));
                    insertAndCheckUpwardExposed(filterVarName(eec->term2->getBaseName()));
                }
            }
        }

        JumpOnComparisonCommand* jocc = node->getComp();
        if (jocc != nullptr)
        {
            if (jocc->term1.type == AbstractCommand::StringType::ID)
            {
                insertAndCheckUpwardExposed(filterVarName(jocc->term1.vptr->getBaseName()));
            }
            if (jocc->term2.type == AbstractCommand::StringType::ID)
            {
                insertAndCheckUpwardExposed(filterVarName(jocc->term2.vptr->getBaseName()));
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
            if ((ac->getType() == CommandType::ASSIGNVAR
                || ac->getType() == CommandType::EXPR
                || ac->getType() == CommandType::CHANGEVAR
                || ac->getType() == CommandType::DECLAREVAR) &&
                isDead(ac->getData()))
            {
                if (ac->getType() == CommandType::DECLAREVAR && usedVars.find(ac->getData()) != usedVars.end())
                {
                    DeclareVarCommand* dvc = static_cast<DeclareVarCommand*>(ac.get());
                    toDeclare.insert({dvc->vt, dvc->getData()});
                }
            }

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

    CFGNode* startNode = cfg.getFirst();
    for (auto& var: toDeclare) startNode->appendDeclatation(var.first, var.second);
}