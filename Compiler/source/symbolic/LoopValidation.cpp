//
// Created by abdul on 26/10/17.
//
#include <sstream>
#include <vector>
#include <algorithm>

#include "../CFGOpt/Loop.h"
#include "SymbolicExecution.h"

//these flags are set if it is possible downstream
#define FINCREASING 1
#define FDECREASING 2
#define FNONE 4
#define FFRESH 8
#define FUNKNOWN 16

using namespace std;

Loop::Loop(CFGNode* entry, CFGNode* last, std::set<CFGNode*> nodeSet, ControlFlowGraph& controlFlowGraph):
            headerNode(entry), nodes(move(nodeSet)), cfg(controlFlowGraph)
{
    if (headerNode->getCompSuccess() != nullptr) comparisonNode = headerNode;
    else if (last->getCompSuccess() != nullptr) comparisonNode = last;
    else if (headerNode->isLastNode() && headerNode->getParentFunction()->getFunctionCalls().size() > 1)
    {
        comparisonNode = headerNode;
        stackBased = true;
    }
    else if (last->isLastNode() && last->getParentFunction()->getFunctionCalls().size() > 1)
    {
        comparisonNode = last;
        stackBased = true;
    }
    else throw "comparison must be at head or exit";

    if (!stackBased) reverse = nodes.find(comparisonNode->getCompSuccess()) == nodes.end();
}

string Loop::getInfo()
{
    std::string outStr = "Header: " + headerNode->getName() + "\nNodes:\n";
    for (CFGNode* node: nodes) outStr += node->getName() + "\n";
    return outStr;
}

void Loop::validate(unordered_map<string, unique_ptr<SearchResult>>& tags)
{
    ChangeMap varChanges; //node->varname->known path through that node where the specified change happens
    SEFPointer sef = make_shared<SymbolicExecution::SymbolicExecutionFringe>(cfg.getReporter());
    sef->symbolicVarSet = make_shared<SymbolicVarSet>(tags[headerNode->getName()]->svs);
    sef->symbolicVarSet->setLoopInit();

    if (stackBased) //i am not proud of this
    {
        vector<CFGNode*> retNodes = comparisonNode->getSuccessorVector();
        sort(retNodes.begin(), retNodes.end());
        vector<CFGNode*> intersect;
        set_intersection(retNodes.begin(), retNodes.end(),
                         nodes.begin(), nodes.end(), inserter(intersect, intersect.begin()));
        if (intersect.size() != 1) throw "should be";
        sef->symbolicStack->pushState((*intersect.cbegin())->getName());
    }

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
        string report = "Potential bad path through the following loop:";
        report += getInfo();
        report += "\nExample:\n" + badExample + " exit loop\n\n";
        cfg.getReporter().addText(report);
    }
}

inline void unionMaps(ChangeMap& cmap, CFGNode* into, CFGNode* from)
{
    map<string, unsigned short int>& intoMap = cmap[into];
    for (const auto& pair : cmap[from]) intoMap[pair.first] |= pair.second;
}

bool Loop::searchNode(CFGNode* node, ChangeMap& varChanges, unordered_map<string, unique_ptr<SearchResult>>& tags,
                      SEFPointer sef, string& badExample, bool headerSeen)
{
    if (nodes.find(node) == nodes.end()) throw "asked to search outside of loop";
    unique_ptr<SearchResult>& thisNodeSR = tags[node->getName()];
    thisNodeSR->resetPoppedCounter();

    for (auto& instr : node->getInstrs())
    {
        if (instr->getType() == CommandType::POP && sef->symbolicStack->isEmpty())
        {
            if (!thisNodeSR->hasPop()) sef->error(Reporter::BAD_STACK_USE,
                                                            "Tried to pop empty stack", instr->getLineNum());
            unique_ptr<SymbolicVariable> RHS = tags[node->getName()]->nextPop();
            if (RHS->getType() != DOUBLE)
            {
                sef->error(Reporter::TYPE, "'" + RHS->getName() + "' (type " + TypeEnumNames[RHS->getType()] +
                                           ") assigned to double", instr->getLineNum());
                return false;
            }

            if (!RHS->isFeasable()) return false;
            RHS->setName(instr->getData());
            sef->symbolicVarSet->defineVar(move(RHS));
        }
        else instr->acceptSymbolicExecution(sef);
    }

    if (headerSeen && node->getName() == comparisonNode->getName())
    {
        if (stackBased) //todo check if stack size decreaced
        {
            bool returnToNodeInLoop = false;
            if (!sef->symbolicStack->isEmpty())
            {
                if (sef->symbolicStack->getTopType() != STATE) throw "tried to return to non state";
                const string& topName = sef->symbolicStack->peekTopName();
                CFGNode* retNode = cfg.getNode(topName);
                if (retNode == nullptr) throw "tried to jump to unknown node";
                returnToNodeInLoop = nodes.find(retNode) != nodes.end();
            }
            if (!returnToNodeInLoop) goodPathFound = true;
            else badExample = sef->printPathConditions() + "(return to state in loop)\n";
        }
        else
        {
            JumpOnComparisonCommand* comp = comparisonNode->getComp();
            SymbolicVariable* varInQuestion = sef->symbolicVarSet->findVar(comp->term1);
            //check if this is a good path
            //first check if we must break the loop condition

            SymbolicVariable::MeetEnum meetStat;
            if (comp->term2Type != AbstractCommand::StringType::ID) meetStat = varInQuestion->canMeet(comp->op, comp->term2);
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
                Relations::Relop op = reverse? Relations::negateRelop(comp->op) : comp->op;
                switch(op)
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
                        if (change != SymbolicVariable::MonotoneEnum::FRESH
                            && meetStat == SymbolicVariable::MeetEnum::MAY) goodPathFound = true;
                        else badExample = sef->printPathConditions() + "(" + comp->term1 + " must meet header condition)\n";
                        break;
                    default:
                        throw "intriguing relop";
                }
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
        if (failNode == nullptr)
        {
            if (sef->symbolicStack->isEmpty())
            {
                goodPathFound = true;
            }
            else if (sef->symbolicStack->getTopType() != STATE) throw "tried to jump to non node";
            else
            {
                CFGNode* nextNode = cfg.getNode(sef->symbolicStack->popState());
                if (nextNode == nullptr) throw "tried to jump to nonexisting node";
                searchNode(nextNode, varChanges, tags, sef, badExample);
                varChanges[node] = varChanges.at(nextNode); //should copy
            }
        }
        else
        {
            if (node->getComp() == nullptr)
            {
                if (nodes.find(failNode) == nodes.end()) throw "unconditional jump should be in the loop";
                searchNode(failNode, varChanges, tags, sef, badExample);
                varChanges[node] = varChanges.at(failNode);
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
                    string RHS;
                    if (jocc->term2Type == AbstractCommand::StringType::ID)
                    {
                        RHS = sef->symbolicVarSet->findVar(jocc->term2)->getConstString();
                    }
                    else RHS = jocc->term2;

                    switch (jumpVar->canMeet(jocc->op, RHS))
                    {
                        case SymbolicVariable::MeetEnum::MUST:
                        {
                            shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                    = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                            string newBadExample;
                            if (sefSucc->addPathCondition(node->getName(), jocc))
                            {
                                if (nodes.find(succNode) == nodes.end()) goodPathFound = true; //left the loop
                                else if (searchNode(succNode, varChanges, tags, sefSucc, newBadExample))
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
                                                if (nodes.find(failNode) == nodes.end()) goodPathFound = true;
                                                else
                                                {
                                                    sefFailure->symbolicVarSet->findVar(jocc->term1)->iterateTo(RHS);
                                                    if (searchNode(failNode, varChanges, tags, sef, badExample)) unionMaps(varChanges, node, failNode);
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
                                if (nodes.find(failNode) == nodes.end()) goodPathFound = true;
                                else if (searchNode(failNode, varChanges, tags, sefFailure, newBadExample))
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
                                                if (nodes.find(succNode) == nodes.end()) goodPathFound = true;
                                                else
                                                {
                                                    sefFailure->symbolicVarSet->findVar(jocc->term1)->iterateTo(RHS);
                                                    if (searchNode(succNode, varChanges, tags, sefSucc, badExample)) unionMaps(varChanges, node, succNode);
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
                                        varChanges[node] = varChanges.at(failNode);
                                    }
                                }
                                else goodPathFound = true;
                            }

                            shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                    = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                            if (sefSucc->addPathCondition(node->getName(), jocc))
                            {
                                if (nodes.find(succNode) == nodes.end()) goodPathFound = true;
                                else if (searchNode(succNode, varChanges, tags, sefSucc, badExample))
                                {
                                    if (firstSearched) unionMaps(varChanges, node, succNode);
                                    else varChanges[node] = varChanges.at(succNode);
                                }
                            }
                            break;
                        }
                        default:
                            throw "weird enum";
                    }
                }
                else //RHS is indeterminate var (todo in far future do the extrapolation stuff here)
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
                                if (nodes.find(node->getCompSuccess()) == nodes.end()) goodPathFound = true;
                                else searchNode(node->getCompSuccess(), varChanges, tags, sefSucc, badExample);
                                varChanges[node] = varChanges.at(failNode);
                            }
                        }

                        case SymbolicVariable::MeetEnum::CANT:
                        {
                            shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFail
                                    = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                            if (sefFail->addPathCondition(node->getName(), jocc, true))
                            {
                                if (nodes.find(node->getCompFail()) == nodes.end()) goodPathFound = true;
                                else
                                {
                                    searchNode(node->getCompFail(), varChanges, tags, sefFail, badExample);
                                    varChanges[node] = varChanges.at(failNode);
                                }
                            }
                        }

                        case SymbolicVariable::MeetEnum::MAY:
                        {
                            shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                    = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                            bool firstSearched = false;
                            if (sefSucc->addPathCondition(node->getName(), jocc))
                            {
                                if (nodes.find(node->getCompSuccess()) == nodes.end()) goodPathFound = true;
                                else if (searchNode(node->getCompSuccess(), varChanges, tags, sefSucc, badExample))
                                {
                                    firstSearched = true;
                                    varChanges[node] = varChanges.at(failNode);
                                }
                            }

                            shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFail
                                    = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                            if (sefFail->addPathCondition(node->getName(), jocc, true))
                            {
                                if (nodes.find(node->getCompFail()) == nodes.end()) goodPathFound = true;
                                else if (searchNode(node->getCompFail(), varChanges, tags, sefFail, badExample))
                                {
                                    if (firstSearched) unionMaps(varChanges, node, node->getCompFail());
                                    else varChanges[node] = varChanges.at(failNode);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}
