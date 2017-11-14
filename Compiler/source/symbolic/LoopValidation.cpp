//
// Created by abdul on 26/10/17.
//
#include <sstream>

#include "../CFGOpt/Loop.h"
#include "SymbolicExecution.h"

//these flags are set if it is possible downstream
#define FINCREASING 1
#define FDECREASING 2
#define FNONE 4
#define FFRESH 8
#define FUNKNOWN 16

using namespace std;

void Loop::validate(unordered_map<string, shared_ptr<SymbolicVarSet>>& tags)
{
    ChangeMap varChanges; //node->varname->known path through that node where the specified change happens
    SEFPointer sef = make_shared<SymbolicExecution::SymbolicExecutionFringe>(reporter);
    sef->symbolicVarSet = make_shared<SymbolicVarSet>(tags[headerNode->getName()]);
    sef->symbolicVarSet->setLoopInit();
    sef->addPathCondition(headerNode->getName(), comp);
    searchNode(headerNode, varChanges, sef);
    if (!goodPathFound)
    {
        string report;
        if (badExample.empty())
        {
            report = "Could not find any good path through the following loop:\n";
            report += getInfo();
        }
        else
        {
            report += "!!!Could only find bad paths through the following loop!!!\n";
            report += getInfo();
            report += "Example of a bad path:\n" + badExample + "exit loop\n";
        }
        reporter.addText(report);
    }
}

inline void unionMaps(ChangeMap& cmap, CFGNode* into, CFGNode* from)
{
    map<string, unsigned short int>& intoMap = cmap[into];
    for (const auto& pair : cmap[from]) intoMap[pair.first] |= pair.second;
}

//todo deal w/ variable comparisons and equality conditions
bool Loop::searchNode(CFGNode* node, ChangeMap& varChanges, SEFPointer sef)
{
    if (nodes.find(node) == nodes.end()) return false;

    for (auto& instr : node->getInstrs()) instr->acceptSymbolicExecution(sef);

    if (node->getName() == exitNode->getName())
    {
        //check if this is a good path
        SymbolicVariable::MonotoneEnum change = sef->symbolicVarSet->findVar(comp->term1)->getMonotonicity();
        switch(comp->op)
        {
            case Relations::LE: case Relations::LT: //needs to be increasing
                if (change == SymbolicVariable::MonotoneEnum::INCREASING) goodPathFound = true;
                else if (change == SymbolicVariable::MonotoneEnum::DECREASING && badExample.empty())
                {
                    badExample = sef->printPathConditions() + "(" + comp->term1 + " decreasing)\n";
                }
                break;
            case Relations::GT: case Relations::GE:
                if (change == SymbolicVariable::MonotoneEnum::DECREASING) goodPathFound = true;
                else if (change == SymbolicVariable::MonotoneEnum::INCREASING && badExample.empty())
                {
                    badExample = sef->printPathConditions() + "(" + comp->term1 + " increasing)\n";
                }
                break;
            default:
                throw "todo";
        }

        map<string, unsigned short int> thisNodeChange;
        for (const auto& symvar : *(sef->symbolicVarSet))
        {
            switch (symvar.second->getMonotonicity())
            {
                case SymbolicVariable::MonotoneEnum::INCREASING:
                    thisNodeChange.insert({symvar.first, FINCREASING});
                    break;
                case SymbolicVariable::MonotoneEnum::DECREASING:
                    thisNodeChange.insert({symvar.first, FDECREASING});
                    break;
                case SymbolicVariable::MonotoneEnum::FRESH:
                    thisNodeChange.insert({symvar.first, FFRESH});
                    break;
                case SymbolicVariable::MonotoneEnum::NONE:
                    thisNodeChange.insert({symvar.first, FNONE});
                    break;
                case SymbolicVariable::MonotoneEnum::UNKNOWN:
                    thisNodeChange.insert({symvar.first, FUNKNOWN});
                    break;
                default:
                    throw "incompatible enum";
            }
        }
        varChanges.insert({node, move(thisNodeChange)});
        return true;
    }
    else //not final node
    {
        CFGNode* failNode = node->getCompFail();
        if (failNode == nullptr) throw "weird collision w/ loop and function";
        if (node->getComp() == nullptr)
        {
            if (nodes.find(failNode) == nodes.end()) throw "unconditional jump should be in the loop";
            searchNode(failNode, varChanges, sef);
            varChanges[node] = varChanges.at(failNode); //should copy
        }
        else
        {
            JumpOnComparisonCommand* jocc = node->getComp();
            CFGNode* succNode = node->getCompSuccess();
            SymbolicVariable*  jumpVar = sef->symbolicVarSet->findVar(jocc->term1);

            if (jumpVar == nullptr) throw "not found";

            if (jocc->term2Type == AbstractCommand::StringType::ID) throw "todo";
            bool closed = (jocc->op == Relations::LE || jocc->op == Relations::GE);
            switch (jumpVar->canMeet(jocc->op, jocc->term2))
            {
                case SymbolicVariable::MeetEnum::MUST:
                {
                    shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                            = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                    sefSucc->addPathCondition(node->getName(), jocc);
                    if (searchNode(succNode, varChanges, sefSucc))
                    {
                        varChanges[node] = varChanges.at(succNode);

                        //check if we can move out of must 'MUST'
                        unsigned short int termChange = varChanges[node][jocc->term1];
                        bool goingAway = (jocc->op == Relations::GT || jocc->op == Relations::GE) && termChange & FDECREASING != 0 ||
                                         (jocc->op == Relations::LT || jocc->op == Relations::LE) && termChange & FINCREASING != 0 ||
                                         (jocc->op == Relations::EQ && termChange & FFRESH != 0);
                        if (goingAway)
                        {
                            shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFailure
                                    = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                            sefFailure->addPathCondition(node->getName(), jocc, true);
                            if (searchNode(failNode, varChanges, sef)) unionMaps(varChanges, node, failNode);
                        }
                        break;
                    }
                }
                case SymbolicVariable::MeetEnum::CANT:
                {
                    shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFailure
                            = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                    sefFailure->addPathCondition(node->getName(), jocc, true);
                    if (searchNode(failNode, varChanges, sefFailure))
                    {
                        varChanges[node] = varChanges.at(failNode);

                        unsigned short int termChange = varChanges[node][jocc->term1];
                        bool goingAway = (jocc->op == Relations::GT || jocc->op == Relations::GE) && termChange & FDECREASING != 0 ||
                                         (jocc->op == Relations::LT || jocc->op == Relations::LE) && termChange & FINCREASING != 0 ||
                                         (jocc->op == Relations::EQ && termChange & FFRESH != 0);
                        if (goingAway)
                        {
                            shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                    = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                            sefSucc->addPathCondition(node->getName(), jocc);
                            if (searchNode(succNode, varChanges, sefSucc)) unionMaps(varChanges, node, succNode);
                        }
                    }
                    break;
                }
                case SymbolicVariable::MeetEnum::MAY:
                {
                    shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFailure
                            = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                    sefFailure->addPathCondition(node->getName(), jocc, true);
                    bool firstInLoop;
                    if (firstInLoop = searchNode(failNode, varChanges, sefFailure))
                    {
                        varChanges[node] = varChanges.at(failNode);
                    }

                    shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                            = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                    sefSucc->addPathCondition(node->getName(), jocc);
                    if (searchNode(succNode, varChanges, sefSucc))
                    {
                        if (firstInLoop) unionMaps(varChanges, node, succNode);
                        else varChanges[node] = varChanges.at(succNode);
                    }
                    break;
                }
                default:
                    throw "weird enum";
            }
        }
    }
}
