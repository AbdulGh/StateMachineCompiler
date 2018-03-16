//
// Created by abdul on 26/10/17.
//
#include <sstream>
#include <vector>
#include <algorithm>
#include <functional>

#include "../CFGOpt/Loop.h"
#include "../compile/VarWrappers.h"
#include "SymbolicExecution.h"

//these flags are set if it is possible downstream
#define FINCREASING 1
#define FDECREASING 2
#define FNONE 4
#define FFRESH 8
#define FUNKNOWN 16

using namespace std;

Loop::Loop(CFGNode* entry, CFGNode* last, std::map<CFGNode*, unsigned long> nodeSet,
           unsigned long tailNumber, ControlFlowGraph& controlFlowGraph):
           headerNode(entry), nodes(move(nodeSet)), cfg(controlFlowGraph), domNum(tailNumber)
{
    if (headerNode->getCompSuccess() != nullptr) comparisonNode = headerNode;
    else if (last->getCompSuccess() != nullptr) comparisonNode = last;
    else if (headerNode->isLastNode())
    {
        comparisonNode = headerNode;
        stackBased = true;
    }
    else if (last->isLastNode())
    {
        comparisonNode = last;
        stackBased = true;
    }
    else throw "comparison must be at head or exit";
}

string Loop::getInfo()
{
    std::string outStr = "Header: " + headerNode->getName() + "\nNodes:\n";
    for (auto pair: nodes)
    {
        outStr += pair.first->getName();
        if (!(pair.second == domNum || pair.first->getName() == headerNode->getName())) outStr += " (nested)";
        outStr += "\n";
    }
    return outStr;
}

void Loop::validate(unordered_map<string, unique_ptr<SearchResult>>& tags)
{
    ChangeMap varChanges; //node->varname->known path through that node where the specified change happens
    SEFPointer sef = make_shared<SymbolicExecution::SymbolicExecutionFringe>(cfg.getReporter());
    unique_ptr<SearchResult>& headerSR = tags[headerNode->getName()];
    sef->symbolicVarSet = move(headerSR->getInitSVS());
    sef->setLoopInit();

    /*if (stackBased) todo next
    {
        vector<CFGNode*> retNodes = comparisonNode->getSuccessorVector();
        sort(retNodes.begin(), retNodes.end());
        vector<CFGNode*> intersect;
        set_intersection(retNodes.begin(), retNodes.end(),
                         nodes.begin(), nodes.end(), inserter(intersect, intersect.begin()));
        if (intersect.size() != 1) throw "should be";
        sef->symbolicStack->pushState((*intersect.cbegin())->getName());
    }*/

    for (auto pair: nodes) varChanges.emplace(pair.first, NodeChangeMap());

    string badExample;
    searchNode(headerNode, varChanges, tags, sef, badExample, false);

    if (!goodPathFound)
    {
        string report;
        if (badExample.empty())
        {
            report = "Could not find any good path through the following loop:\n";
            report += getInfo() + "\n";
        }
        else
        {
            report += "!!!Could only find bad paths through the following loop!!!\n";
            report += getInfo();
            report += "Example of a bad path:\n" + badExample + "exit loop\n\n";
        }
        cfg.getReporter().addText(report);
    }
    else if (!badExample.empty())
    {
        string report = "=Potential bad path through the following loop:\n";
        report += getInfo();
        report += "\nExample:\n" + badExample + " exit loop\n-----\n";
        cfg.getReporter().addText(report);
    }
}

inline void mergeMaps(NodeChangeMap& intoMap, NodeChangeMap& fromMap)
{
    for (const auto& pair : fromMap)
    {
        auto it = intoMap.find(pair.first);
        if (it != intoMap.end()) intoMap[pair.first] = pair.second;
        else intoMap[pair.first] |= pair.second;
    }
}

bool Loop::searchNode(CFGNode* node, ChangeMap& varChanges, unordered_map<string, unique_ptr<SearchResult>>& tags,
                      SEFPointer sef, string& badExample, bool headerSeen)
{
    if (nodes.find(node) == nodes.end()) throw "asked to search outside of loop";
    unique_ptr<SearchResult>& thisNodeSR = tags[node->getName()];
    thisNodeSR->resetPoppedCounter();

    for (auto& instr : node->getInstrs())
    {
        if (instr->getType() == CommandType::POP)
        {
            if (sef->symbolicStack->isEmpty())
            {
                if (!instr->getData().empty())
                {
                    unique_ptr<SymbolicVariable> popped = thisNodeSR->popVar();
                    popped->setName(instr->getData());
                    popped->loopInit();
                    sef->symbolicVarSet->addVar(move(popped));
                }
            }
            else
            {
                thisNodeSR->incrementPoppedCounter();
                instr->acceptSymbolicExecution(sef);
            }
        }
        else instr->acceptSymbolicExecution(sef);
    }

    auto generateNodeChanges = [&varChanges, &sef, node] () -> void
    {
        map<string, unsigned short int> thisNodeChange;
        for (const auto& symvar : sef->symbolicVarSet->getAllVars()) //todo make this more efficient in the style of (broken) SVSIterator
        {
            if (thisNodeChange.find(symvar.first) != thisNodeChange.end()) continue;
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
        } //todo without thisNodeChange
        mergeMaps(varChanges.at(node), thisNodeChange);
    };

    if (headerSeen && node->getName() == comparisonNode->getName())
    {
        if (stackBased) //todo check if stack size decreased
        {
            bool returnToNodeInLoop = false;
            if (!sef->symbolicStack->isEmpty())
            {
                if (sef->symbolicStack->peekTopType() != SymbolicStackMemberType::STATE) throw "tried to return to non state";
                const string& topName = sef->symbolicStack->peekTopName();
                CFGNode* retNode = cfg.getNode(topName);
                if (retNode == nullptr) throw "tried to jump to unknown node";
                returnToNodeInLoop = nodes.find(retNode) != nodes.end();
            }
            if (!returnToNodeInLoop) goodPathFound = true;
            else badExample = sef->printPathConditions() + "(return to state in loop)\n";
            generateNodeChanges();
        }
        else
        {
            string newBadExample;
            bool noGood = true;
            for (SymbolicExecution::Condition condition : sef->getConditions())
            {
                SymbolicVariable* varInQuestion = sef->symbolicVarSet->findVar(condition.lhs);
                //check if this is a good path
                //first check if we must break a loop condition

                SymbolicVariable::MeetEnum meetStat;
                if (AbstractCommand::getStringType(condition.rhs) != StringType::ID)
                {
                    meetStat = varInQuestion->canMeet(condition.comp, condition.rhs);
                }
                else
                {
                    SymbolicVariable* rhVar = sef->symbolicVarSet->findVar(condition.rhs);
                    if (!rhVar) throw runtime_error("Unknown var '" + condition.rhs + "'");
                    meetStat = varInQuestion->canMeet(condition.comp, rhVar);
                }

                if (meetStat == SymbolicVariable::MeetEnum::CANT) noGood = false;
                else
                {
                    SymbolicVariable::MonotoneEnum change = varInQuestion->getMonotonicity();

                    if (change == SymbolicVariable::MonotoneEnum::FRESH)
                    {
                        newBadExample = sef->printPathConditions() + "(" + condition.lhs + " unchanging)\n";
                    }
                    else
                    {
                        switch(condition.comp)
                        {
                            case Relations::LE: case Relations::LT: //needs to be increasing
                                if (change == SymbolicVariable::MonotoneEnum::INCREASING) noGood = false;
                                else if (change == SymbolicVariable::MonotoneEnum::DECREASING && newBadExample.empty())
                                {
                                    newBadExample = sef->printPathConditions() + "(" + condition.lhs + " decreasing)\n";
                                }
                                break;
                            case Relations::GT: case Relations::GE:
                                if (change == SymbolicVariable::MonotoneEnum::DECREASING) noGood = false;
                                else if (change == SymbolicVariable::MonotoneEnum::INCREASING && newBadExample.empty())
                                {
                                    newBadExample = sef->printPathConditions() + "(" + condition.lhs + " increasing)\n";
                                }
                                break;
                            case Relations::EQ: case Relations::NEQ:
                                if (change != SymbolicVariable::MonotoneEnum::FRESH
                                    && meetStat == SymbolicVariable::MeetEnum::MAY) noGood = false;
                                else newBadExample = sef->printPathConditions() + "(" + condition.lhs + " must meet header condition)\n";
                                break;
                            default:
                                throw "intriguing relop";
                        }
                    }
                }
            }
            if (noGood) badExample = newBadExample;
            else goodPathFound = true;
        }
        generateNodeChanges();
        return true;
    }
    else //not done w/ this path
    {
        CFGNode* failNode = node->getCompFail();
        if (node->getComp() == nullptr)
        {
            if (failNode == nullptr)
            {
                if (sef->symbolicStack->isEmpty())
                {
                    generateNodeChanges();
                    goodPathFound = true;
                }
                else if (sef->symbolicStack->peekTopType() != SymbolicStackMemberType::STATE) throw "tried to jump to non node";
                else
                {
                    SEFPointer newSEF = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                    CFGNode* nextNode = cfg.getNode(newSEF->symbolicStack->popState());
                    if (nextNode == nullptr) throw "tried to jump to nonexisting node";
                    else if (nodes.find(nextNode) == nodes.end()) goodPathFound = true;
                    else
                    {
                        searchNode(nextNode, varChanges, tags, newSEF, badExample);
                        mergeMaps(varChanges.at(node), varChanges.at(nextNode));
                    }
                }
            }
            else
            {
                if (nodes.find(failNode) == nodes.end()) throw "unconditional jump should be in the loop";
                bool t = searchNode(failNode, varChanges, tags, sef, badExample);
                mergeMaps(varChanges.at(node), varChanges.at(failNode));
                return t;
            }
        }

        else
        {
            JumpOnComparisonCommand* jocc = node->getComp();
            CFGNode* succNode = node->getCompSuccess();
            GottenVarPtr<SymbolicVariable> lhVar = jocc->term1.vptr->getSymbolicVariable(sef.get());
            if (!lhVar) throw "not found";

            GottenVarPtr<SymbolicVariable> rhVar(nullptr);
            bool rhconst = true;
            if (jocc->term2.type == StringType::ID)
            {
                auto lvalue = jocc->term2.vptr->getSymbolicVariable(sef.get());
                rhVar.become(lvalue);
                if (!rhVar) throw runtime_error("Unknown var '" + string(jocc->term2) + "'");
                rhconst = rhVar->isDetermined();
            }

            if (rhconst)
            {
                string RHS;
                if (jocc->term2.type == StringType::ID) RHS = rhVar->getConstString();
                else RHS = string(jocc->term2);

                switch (lhVar->canMeet(jocc->op, RHS))
                {
                    case SymbolicVariable::MeetEnum::MUST:
                    {
                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        string newBadExample;
                        if (sefSucc->addPathCondition(node->getName(), jocc))
                        {
                            if (nodes.find(succNode) == nodes.end()) //left the loop
                            {
                                generateNodeChanges();
                                goodPathFound = true;
                            }
                            else if (searchNode(succNode, varChanges, tags, sefSucc, newBadExample))
                            {
                                mergeMaps(varChanges.at(node), varChanges.at(succNode));

                                //check if we can move out of must 'MUST'
                                if (lhVar->isIncrementable())
                                {
                                    unsigned short int termChange = varChanges.at(node)[string(jocc->term1)];
                                    bool goingAway = (jocc->op == Relations::GT || jocc->op == Relations::GE) && termChange & FDECREASING != 0 ||
                                                     (jocc->op == Relations::LT || jocc->op == Relations::LE) && termChange & FINCREASING != 0 ||
                                                     (jocc->op == Relations::EQ && termChange & FFRESH != 0);
                                    if (goingAway)
                                    {
                                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFailure
                                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                                        if (sefFailure->addPathCondition(node->getName(), jocc, true))
                                        {
                                            if (nodes.find(failNode) == nodes.end())
                                            {
                                                generateNodeChanges();
                                                goodPathFound = true;
                                            }
                                            else
                                            {
                                                jocc->term1.vptr->getSymbolicVariable(sefFailure.get())->iterateTo(RHS);
                                                if (searchNode(failNode, varChanges, tags, sef, badExample))
                                                {
                                                    mergeMaps(varChanges.at(node), varChanges.at(failNode));
                                                }
                                            }
                                        }
                                    }
                                    if (badExample.empty()) badExample = newBadExample;
                                    break;
                                }
                            }
                        }
                    }
                    case SymbolicVariable::MeetEnum::CANT:
                    {
                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFailure
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        string newBadExample;
                        if (sefFailure->addPathCondition(node->getName(), jocc, true))
                        {
                            if (nodes.find(failNode) == nodes.end())
                            {
                                generateNodeChanges();
                                goodPathFound = true;
                            }
                            else if (searchNode(failNode, varChanges, tags, sefFailure, newBadExample))
                            {
                                mergeMaps(varChanges.at(node), varChanges.at(failNode));

                                if (lhVar->isIncrementable())
                                {
                                    unsigned short int termChange = varChanges.at(node)[string(jocc->term1)];
                                    bool goingAway = (jocc->op == Relations::GT || jocc->op == Relations::GE) && termChange & FINCREASING != 0 ||
                                                     (jocc->op == Relations::LT || jocc->op == Relations::LE) && termChange & FDECREASING != 0 ||
                                                     (jocc->op == Relations::EQ && termChange & FFRESH != 0);
                                    if (goingAway)
                                    {
                                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                                        if (sefSucc->addPathCondition(node->getName(), jocc))
                                        {
                                            if (nodes.find(succNode) == nodes.end())
                                            {
                                                generateNodeChanges();
                                                goodPathFound = true;
                                            }
                                            else
                                            {
                                                jocc->term1.vptr->getSymbolicVariable(sefSucc.get())->iterateTo(RHS);
                                                if (searchNode(succNode, varChanges, tags, sefSucc, badExample))
                                                {
                                                    mergeMaps(varChanges.at(node), varChanges.at(succNode));
                                                }
                                            }
                                        }
                                    }
                                    if (badExample.empty()) badExample = newBadExample;
                                }
                            }
                        }
                        break;
                    }
                    case SymbolicVariable::MeetEnum::MAY:
                    {
                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFailure
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        bool firstSearched = false;
                        if (sefFailure->addPathCondition(node->getName(), jocc, true))
                        {
                            if (nodes.find(failNode) != nodes.end())
                            {
                                if (firstSearched = searchNode(failNode, varChanges, tags, sefFailure, badExample))
                                {
                                    mergeMaps(varChanges.at(node), varChanges.at(failNode));
                                }
                            }
                            else
                            {
                                generateNodeChanges();
                                goodPathFound = true;
                            }
                        }

                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        if (sefSucc->addPathCondition(node->getName(), jocc))
                        {
                            if (nodes.find(succNode) == nodes.end())
                            {
                                generateNodeChanges();
                                goodPathFound = true;
                            }
                            else if (searchNode(succNode, varChanges, tags, sefSucc, badExample))
                            {
                                if (firstSearched) mergeMaps(varChanges.at(node), varChanges.at(succNode));
                                else mergeMaps(varChanges.at(node), varChanges.at(succNode));
                            }
                        }
                        break;
                    }
                    default:
                        throw "weird enum";
                }
            }
            else //rhs is indeterminate var (could also do the extrapolation stuff here)
            {
                switch (lhVar->canMeet(jocc->op, rhVar.get()))
                {
                    case SymbolicVariable::MeetEnum::MUST:
                    {
                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        if (sefSucc->addPathCondition(node->getName(), jocc))
                        {
                            if (nodes.find(node->getCompSuccess()) == nodes.end())
                            {
                                generateNodeChanges();
                                goodPathFound = true;
                            }
                            else searchNode(node->getCompSuccess(), varChanges, tags, sefSucc, badExample);
                            mergeMaps(varChanges.at(node), varChanges.at(failNode));
                        }
                    }

                    case SymbolicVariable::MeetEnum::CANT:
                    {
                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFail
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        if (sefFail->addPathCondition(node->getName(), jocc, true))
                        {
                            if (nodes.find(node->getCompFail()) == nodes.end())
                            {
                                generateNodeChanges();
                                goodPathFound = true;
                            }
                            else
                            {
                                searchNode(node->getCompFail(), varChanges, tags, sefFail, badExample);
                                mergeMaps(varChanges.at(node), varChanges.at(failNode));
                            }
                        }
                    }

                    case SymbolicVariable::MeetEnum::MAY:
                    {
                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        if (sefSucc->addPathCondition(node->getName(), jocc))
                        {
                            if (nodes.find(node->getCompSuccess()) == nodes.end())
                            {
                                generateNodeChanges();
                                goodPathFound = true;
                            }
                            else if (searchNode(node->getCompSuccess(), varChanges, tags, sefSucc, badExample))
                            {
                                mergeMaps(varChanges.at(node), varChanges.at(node->getCompSuccess()));
                            }
                        }

                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFail
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        if (sefFail->addPathCondition(node->getName(), jocc, true))
                        {
                            if (nodes.find(node->getCompFail()) == nodes.end())
                            {
                                generateNodeChanges();
                                goodPathFound = true;
                            }
                            else if (searchNode(node->getCompFail(), varChanges, tags, sefFail, badExample))
                            {
                                mergeMaps(varChanges.at(node), varChanges.at(failNode));
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}
