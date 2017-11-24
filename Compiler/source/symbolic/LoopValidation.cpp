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
    //sef->addPathCondition(headerNode->getName(), comp);
    string badExample;
    searchNode(headerNode, varChanges, sef, badExample);
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

//todo deal w/ equality conditions
//todo test this with pushes/shoves
bool Loop::searchNode(CFGNode* node, ChangeMap& varChanges, SEFPointer sef, string& badExample)
{
    if (nodes.find(node) == nodes.end()) return false;

    for (auto& instr : node->getInstrs()) instr->acceptSymbolicExecution(sef);

    if (node->getName() == exitNode->getName())
    {
        SymbolicVariable* varInQuestion = sef->symbolicVarSet->findVar(comp->term1);
        //check if this is a good path
        //first check if we must break the loop condition

        SymbolicVariable::MeetEnum meetStat;
        if (comp->term2Type != AbstractCommand::StringType::ID)
        {
            meetStat = varInQuestion->canMeet(comp->op, comp->term2);
        }
        else
        {
            SymbolicVariable* rhVar = sef->symbolicVarSet->findVar(comp->term1);
            if (!rhVar) throw runtime_error("Unknown var '" + comp->term2 + "'");
            meetStat = varInQuestion->canMeet(comp->op, rhVar);
        }

        if (meetStat == SymbolicVariable::MeetEnum::CANT) goodPathFound = true;
        else
        {
            SymbolicVariable::MonotoneEnum change = varInQuestion->getMonotonicity();
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
                case Relations::EQ: case Relations::NEQ:
                    throw "todo";
            }
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
            searchNode(failNode, varChanges, sef, badExample);
            varChanges[node] = varChanges.at(failNode); //should copy
        }
        else
        {
            JumpOnComparisonCommand* jocc = node->getComp();
            CFGNode* succNode = node->getCompSuccess();
            SymbolicVariable*  jumpVar = sef->symbolicVarSet->findVar(jocc->term1);

            if (jumpVar == nullptr) throw "not found";

            if (jocc->term2Type != AbstractCommand::StringType::ID ||
                    sef->symbolicVarSet->findVar(jocc->term2)->isDetermined())
            {
                if (jocc->term2Type == AbstractCommand::StringType::ID)
                {
                    jocc->setTerm2(sef->symbolicVarSet->findVar(jocc->term2)->getConstString());
                }

                bool closed = (jocc->op == Relations::LE || jocc->op == Relations::GE);
                switch (jumpVar->canMeet(jocc->op, jocc->term2))
                {
                    case SymbolicVariable::MeetEnum::MUST:
                    {
                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        string newBadExample;
                        if (sefSucc->addPathCondition(node->getName(), jocc) &&
                            searchNode(succNode, varChanges, sefSucc, newBadExample))
                        {
                            varChanges[node] = varChanges.at(succNode);

                            //check if we can move out of must 'MUST'
                            if (jumpVar->isIncrementable())
                            {
                                unsigned short int termChange = varChanges[node][jocc->term1];
                                bool goingAway = (jocc->op == Relations::GT || jocc->op == Relations::GE) && termChange & FDECREASING != 0 ||
                                                 (jocc->op == Relations::LT || jocc->op == Relations::LE) && termChange & FINCREASING != 0 ||
                                                 (jocc->op == Relations::EQ && termChange & FFRESH != 0);
                                if (goingAway)
                                {
                                    shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFailure
                                            = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                                    if (sefFailure->addPathCondition(node->getName(), jocc, true))
                                    {
                                        if (jocc->term2Type == AbstractCommand::StringType::ID) throw "todo";
                                        sefFailure->symbolicVarSet->findVar(jocc->term1)->iterateTo(jocc->term2);
                                        if (searchNode(failNode, varChanges, sef, badExample)) unionMaps(varChanges, node, failNode);
                                    }
                                    else if (badExample.empty()) badExample = newBadExample;
                                }
                                else if (badExample.empty()) badExample = newBadExample;
                                break;
                            }
                        }
                    }
                    case SymbolicVariable::MeetEnum::CANT:
                    {
                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFailure
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        string newBadExample;
                        if (sefFailure->addPathCondition(node->getName(), jocc, true) &&
                            searchNode(failNode, varChanges, sefFailure, newBadExample))
                        {
                            varChanges[node] = varChanges.at(failNode);

                            if (jumpVar->isIncrementable())
                            {
                                unsigned short int termChange = varChanges[node][jocc->term1];
                                bool goingAway = (jocc->op == Relations::GT || jocc->op == Relations::GE) && termChange & FINCREASING != 0 ||
                                                 (jocc->op == Relations::LT || jocc->op == Relations::LE) && termChange & FDECREASING != 0 ||
                                                 (jocc->op == Relations::EQ && termChange & FFRESH != 0);
                                if (goingAway)
                                {
                                    shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                            = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                                    if (sefSucc->addPathCondition(node->getName(), jocc))
                                    {
                                        if (jocc->term2Type == AbstractCommand::StringType::ID) throw "todo";
                                        sefFailure->symbolicVarSet->findVar(jocc->term1)->iterateTo(jocc->term2);
                                        if (searchNode(succNode, varChanges, sefSucc, badExample)) unionMaps(varChanges, node, succNode);
                                    }
                                    else if (badExample.empty()) badExample = newBadExample;
                                }
                                else if (badExample.empty()) badExample = newBadExample;
                            }
                        }
                        break;
                    }
                    case SymbolicVariable::MeetEnum::MAY:
                    {
                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFailure
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        bool firstInLoop = false;
                        if (sefFailure->addPathCondition(node->getName(), jocc, true) &&
                            (firstInLoop = searchNode(failNode, varChanges, sefFailure, badExample)))
                        {
                            varChanges[node] = varChanges.at(failNode);
                        }

                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        if (sefSucc->addPathCondition(node->getName(), jocc) &&
                            searchNode(succNode, varChanges, sefSucc, badExample))
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
            else //RHS is indeterminate var
            {
                SymbolicVariable* RHS = sef->symbolicVarSet->findVar(jocc->term2);
                if (!RHS) throw runtime_error("Unknown var '" + jocc->term2 + "'");
                switch (jumpVar->canMeet(jocc->op, RHS))
                {
                    case SymbolicVariable::MeetEnum::MUST:
                    {
                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        if (sefSucc->addPathCondition(node->getName(), jocc))
                        {
                            searchNode(node->getCompSuccess(),varChanges, sefSucc, badExample);
                        }
                    }

                    case SymbolicVariable::MeetEnum::CANT:
                    {
                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFail
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        if (sefFail->addPathCondition(node->getName(), jocc, true))
                        {
                            searchNode(node->getCompFail(),varChanges, sefFail, badExample);
                        }
                    }

                    case SymbolicVariable::MeetEnum::MAY:
                    {
                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        if (sefSucc->addPathCondition(node->getName(), jocc))
                        {
                            searchNode(node->getCompSuccess(),varChanges, sefSucc, badExample);
                        }

                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFail
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        if (sefFail->addPathCondition(node->getName(), jocc, true))
                        {
                            searchNode(node->getCompFail(),varChanges, sefFail, badExample);
                        }
                    }
                }
            }
        }
    }
    return true;
}
