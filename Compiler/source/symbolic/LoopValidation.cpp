//
// Created by abdul on 26/10/17.
//
#include <sstream>
#include <vector>
#include <algorithm>
#include <functional>

#include "../CFGOpt/Loop.h"
#include "VarWrappers.h"
#include "SymbolicExecution.h"

//these flags are set if it is possible downstream
#define FINCREASING 1
#define FDECREASING 2
#define FNONE 4
#define FFRESH 8
#define FUNKNOWN 16

using namespace std;

Loop::Loop(CFGNode* entry, CFGNode* last, std::map<CFGNode*, Loop*> nodeSet,
           unsigned long tailNumber, ControlFlowGraph& controlFlowGraph):
           headerNode(entry), nodes(move(nodeSet)), cfg(controlFlowGraph),
           domNum(tailNumber) {}

string Loop::getInfo(bool nested, string indent)
{
    std::string outStr = "------------\n";
    outStr += indent + "Header: " + headerNode->getName() + "\n";
    outStr += indent + "Nodes:\n";
    for (auto pair: nodes)
    {
        outStr += indent + pair.first->getName();
        if (pair.second) outStr += " (nested)";
        outStr += "\n";
    }
    if (nested)
    {
        indent += "--";
        for (auto& child : children) outStr += child->getInfo(true, indent);
        return outStr;
    }
}

void Loop::addChild(std::unique_ptr<Loop> child)
{
    if (!children.insert(move(child)).second) throw runtime_error("Already have this child");
}

void Loop::setNodeNesting(CFGNode* node, Loop* child)
{
    nodes[node] = child;
}

void Loop::validate(unordered_map<string, unique_ptr<SearchResult>>& tags)
{
    for (auto& child : children) child->validate(tags);

    ChangeMap varChanges; //node->varname->known path through that node where the specified change happens
    SEFPointer sef = make_shared<SymbolicExecution::SymbolicExecutionFringe>(cfg.getReporter());
    unique_ptr<SearchResult>& headerSR = tags[headerNode->getName()];
    sef->symbolicVarSet = headerSR->getInitSVS();
    sef->setLoopInit();

    /*if (stackBased) todo
    {
        vector<CFGNode*> retNodes = comparisonNode->getSuccessorVector();
        sort(retNodes.begin(), retNodes.end());
        vector<CFGNode*> intersect;
        set_intersection(retNodes.begin(), retNodes.end(),
                         nodes.begin(), nodes.end(), inserter(intersect, intersect.begin()));
        if (intersect.size() != 1) throw std::runtime_error("should be");
        sef->symbolicStack->pushState((*intersect.cbegin())->getName());
    }*/

    for (auto pair: nodes) varChanges.emplace(pair.first, NodeChangeMap());

    searchNode(headerNode, varChanges, tags, sef,  false);

    if (!goodPathFound)
    {
        string report = "Could not find any good path through the following loop:\n";
        report += getInfo() + "\n";
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
                      SEFPointer sef,  bool headerSeen)
{
    auto it = nodes.find(node);
    if (it == nodes.end()) throw std::runtime_error("asked to search outside of loop");

    unique_ptr<SearchResult>& thisNodeSR = tags[node->getName()];

    thisNodeSR->resetPoppedCounter();

    bool inNested;
    if (it->second != nullptr)
    {
        inNested = true;
        sef->symbolicVarSet->unionSVS(thisNodeSR->getInitSVS().get());
    }
    else inNested = false;

    for (auto& instr : node->getInstrs())
    {
        if (instr->getType() == CommandType::POP && instr->getVarWrapper())
        {
            if (sef->symbolicStack->isEmpty())
            {
                unique_ptr<SymbolicDouble> popped = thisNodeSR->popVar();
                instr->getVarWrapper()->setSymbolicDouble(sef.get(), popped.get());
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
                case SymbolicDouble::MonotoneEnum::INCREASING:
                    thisNodeChange.insert({symvar.first, FINCREASING});
                    break;
                case SymbolicDouble::MonotoneEnum::DECREASING:
                    thisNodeChange.insert({symvar.first, FDECREASING});
                    break;
                case SymbolicDouble::MonotoneEnum::FRESH:
                    thisNodeChange.insert({symvar.first, FFRESH});
                    break;
                case SymbolicDouble::MonotoneEnum::NONE:
                    thisNodeChange.insert({symvar.first, FNONE});
                    break;
                case SymbolicDouble::MonotoneEnum::UNKNOWN:
                    thisNodeChange.insert({symvar.first, FUNKNOWN});
                    break;
                default:
                    throw std::runtime_error("incompatible enum");
            }
        } //todo without thisNodeChange
        mergeMaps(varChanges.at(node), thisNodeChange);
    };

    if (headerSeen && node->getName() == headerNode->getName())
    {
        string badExample;
        if (stackBased) //todo check if stack size decreased
        {
            bool returnToNodeInLoop = false;
            if (!sef->symbolicStack->isEmpty())
            {
                if (sef->symbolicStack->peekTopType() != SymbolicStackMemberType::STATE) throw std::runtime_error("tried to return to non state");
                const string& topName = sef->symbolicStack->peekTopName();
                CFGNode* retNode = cfg.getNode(topName);
                if (retNode == nullptr) throw std::runtime_error("tried to jump to unknown node");
                returnToNodeInLoop = nodes.find(retNode) != nodes.end();
            }
            if (!returnToNodeInLoop) goodPathFound = true;
            else badExample = sef->printPathConditions() + "(return to state in loop)\n";
            generateNodeChanges();
        }
        else //todo deal with nonmonotone vars
        {
            bool noGood = true;
            bool noConditional = true;

            for (SymbolicExecution::Condition condition : sef->getConditions())
            {
                if (condition.unconditional) continue;
                else noConditional = false;

                badExample += "Visit " + condition.location + ", branch " + condition.toString() + "\n";

                GottenVarPtr<SymbolicDouble> varInQuestion
                        = condition.lhs.getVarWrapper()->getSymbolicDouble(sef.get());
                //check if this is a good path
                //first check if we must break a loop condition

                SymbolicDouble::MeetEnum meetStat;
                if (!condition.rhs.isHolding())
                {
                    meetStat = varInQuestion->canMeet(condition.comp, condition.rhs.getLiteral());
                }
                else
                {
                    GottenVarPtr<SymbolicDouble> rhVar
                            = condition.rhs.getVarWrapper()->getSymbolicDouble(sef.get());
                    meetStat = varInQuestion->canMeet(condition.comp, rhVar.get());
                }

                if (meetStat == SymbolicDouble::MeetEnum::CANT) noGood = false;
                else
                {
                    SymbolicDouble::MonotoneEnum change;

                    if (condition.rhs.isHolding())
                    {
                        long double slowest; long double fastest;
                        const VarWrapper* rhsVW = condition.rhs.getVarWrapper();
                        GottenVarPtr<SymbolicDouble> rhs = rhsVW->getSymbolicDouble(sef.get());
                        varInQuestion->getRelativeVelocity(rhs.get(), slowest, fastest);
                        if (slowest == 0 && fastest == 0) change = SymbolicDouble::MonotoneEnum::FRESH;
                        else if (slowest >= 0) change = SymbolicDouble::MonotoneEnum::INCREASING;
                        else if (fastest <= 0) change = SymbolicDouble::MonotoneEnum::DECREASING;
                        else change = SymbolicDouble::MonotoneEnum::NONE;
                    }

                    else change = varInQuestion->getMonotonicity();

                    if (change == SymbolicDouble::MonotoneEnum::FRESH)
                    {
                        badExample += "(" + string(condition.lhs) + " unchanging compared to RHS)\n";
                    }
                    else
                    {
                        switch(condition.comp)
                        {
                            case Relations::LE: case Relations::LT: //needs to be increasing
                                if (change == SymbolicDouble::MonotoneEnum::INCREASING) noGood = false;
                                else if (change == SymbolicDouble::MonotoneEnum::DECREASING)
                                {
                                    badExample += "(" + string(condition.lhs)+ " decreasing compared to RHS)\n";
                                }
                                break;
                            case Relations::GT: case Relations::GE:
                                if (change == SymbolicDouble::MonotoneEnum::DECREASING) noGood = false;
                                else if (change == SymbolicDouble::MonotoneEnum::INCREASING)
                                {
                                    badExample += "(" + string(condition.lhs) + " increasing compared to RHS)\n";
                                }
                                break;
                            case Relations::EQ: case Relations::NEQ:
                                if (change != SymbolicDouble::MonotoneEnum::FRESH) continue;
                                else badExample += "(" + string(condition.lhs) + " must meet header condition)\n";
                                break;
                            default:
                                throw std::runtime_error("intriguing relop");
                        }
                    }
                }
            }
            if (noConditional) badExample = sef->printPathConditions() + "(no conditionals - possibly optimised away)";
            if (noGood)
            {
                string report = "Potential bad path through the following loop:\n";
                report += getInfo();
                if (badExample.empty()) report += "Example: " + sef->printPathConditions() + "\n";
                else report += "\nExample:\n" + badExample + "\n-----\n";
                cfg.getReporter().addText(report);
            }
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
                else if (sef->symbolicStack->peekTopType() != SymbolicStackMemberType::STATE) throw std::runtime_error("tried to jump to non node");
                else
                {
                    SEFPointer newSEF = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                    CFGNode* nextNode = cfg.getNode(newSEF->symbolicStack->popState());
                    if (nextNode == nullptr) throw std::runtime_error("tried to jump to nonexisting node");
                    else
                    {
                        auto it = nodes.find(nextNode);
                        if (it == nodes.end() || inNested && it->second != nullptr) goodPathFound = true;
                        else
                        {
                            searchNode(nextNode, varChanges, tags, newSEF);
                            mergeMaps(varChanges.at(node), varChanges.at(nextNode));
                        }
                    }
                }
            }
            else
            {
                if (nodes.find(failNode) == nodes.end()) throw std::runtime_error("unconditional jump should be in the loop");
                sef->addPathCondition(node->getName(), nullptr);
                bool t = searchNode(failNode, varChanges, tags, sef);
                mergeMaps(varChanges.at(node), varChanges.at(failNode));
                return t;
            }
        }

        else
        {
            JumpOnComparisonCommand* jocc = node->getComp();
            CFGNode* succNode = node->getCompSuccess();
            GottenVarPtr<SymbolicDouble> lhVar = jocc->term1.getVarWrapper()->getSymbolicDouble(sef.get());
            if (!lhVar) throw std::runtime_error("not found");

            GottenVarPtr<SymbolicDouble> rhVar(nullptr);
            bool rhconst = true;
            if (jocc->term2.getType() == StringType::ID)
            {
                auto lvalue = jocc->term2.getVarWrapper()->getSymbolicDouble(sef.get());
                rhVar.become(lvalue);
                if (!rhVar) throw runtime_error("Unknown var '" + string(jocc->term2) + "'");
                rhconst = rhVar->isDetermined();
            }

            if (rhconst)
            {
                double RHS;
                if (jocc->term2.getType() == StringType::ID) RHS = rhVar->getConstValue();
                else RHS = jocc->term2.getLiteral();

                switch (lhVar->canMeet(jocc->op, RHS))
                {
                    case SymbolicDouble::MeetEnum::MUST:
                    {
                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        string newBadExample;
                        if (sefSucc->addPathCondition(node->getName(), jocc))
                        {
                            auto it = nodes.find(succNode);
                            if (it == nodes.end() || inNested && it->second != nullptr) //left the loop
                            {
                                generateNodeChanges();
                                goodPathFound = true;
                            }
                            else
                            {
                                if (searchNode(succNode, varChanges, tags, sefSucc))
                                {
                                    mergeMaps(varChanges.at(node), varChanges.at(succNode));

                                    //check if we can move out of must 'MUST'
                                    unsigned short int termChange = varChanges.at(node)[string(jocc->term1)];
                                    bool goingAway = (jocc->op == Relations::GT || jocc->op == Relations::GE) &&
                                                     termChange & FDECREASING != 0 ||
                                                     (jocc->op == Relations::LT || jocc->op == Relations::LE) &&
                                                     termChange & FINCREASING != 0 ||
                                                     (jocc->op == Relations::EQ && termChange & FFRESH != 0);
                                    if (goingAway)
                                    {
                                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFailure
                                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                                        if (sefFailure->addPathCondition(node->getName(), jocc, true))
                                        {
                                            it = nodes.find(failNode);
                                            if (it == nodes.end() || inNested && it->second != nullptr)
                                            {
                                                generateNodeChanges();
                                                goodPathFound = true;
                                            }
                                            else
                                            {
                                                jocc->term1.getVarWrapper()->getSymbolicDouble(
                                                        sefFailure.get())->iterateTo(RHS);
                                                if (searchNode(failNode, varChanges, tags, sef))
                                                {
                                                    mergeMaps(varChanges.at(node), varChanges.at(failNode));
                                                }
                                            }
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    case SymbolicDouble::MeetEnum::CANT:
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
                            else
                            {
                                if (searchNode(failNode, varChanges, tags, sefFailure))
                                {
                                    mergeMaps(varChanges.at(node), varChanges.at(failNode));
                                    unsigned short int termChange = varChanges.at(node)[string(jocc->term1)];
                                    bool goingAway = (jocc->op == Relations::GT || jocc->op == Relations::GE) &&
                                                     termChange & FINCREASING != 0 ||
                                                     (jocc->op == Relations::LT || jocc->op == Relations::LE) &&
                                                     termChange & FDECREASING != 0 ||
                                                     (jocc->op == Relations::EQ && termChange & FFRESH != 0);
                                    if (goingAway)
                                    {
                                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                                        if (sefSucc->addPathCondition(node->getName(), jocc))
                                        {
                                            it = nodes.find(succNode);
                                            if (it == nodes.end() || inNested && it->second != nullptr)
                                            {
                                                generateNodeChanges();
                                                goodPathFound = true;
                                            } else
                                            {
                                                jocc->term1.getVarWrapper()->getSymbolicDouble(
                                                        sefSucc.get())->iterateTo(RHS);
                                                if (searchNode(succNode, varChanges, tags, sefSucc))
                                                {
                                                    mergeMaps(varChanges.at(node), varChanges.at(succNode));
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case SymbolicDouble::MeetEnum::MAY:
                    {
                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFailure
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        bool firstSearched = false;
                        if (sefFailure->addPathCondition(node->getName(), jocc, true))
                        {
                            it = nodes.find(failNode);
                            if (it == nodes.end() || inNested && it->second != nullptr)
                            {
                                generateNodeChanges();
                                goodPathFound = true;
                            }
                            else
                            {
                                if (firstSearched = searchNode(failNode, varChanges, tags,
                                                               sefFailure))
                                {
                                    mergeMaps(varChanges.at(node), varChanges.at(failNode));
                                }
                            }
                        }

                        shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                        if (sefSucc->addPathCondition(node->getName(), jocc))
                        {
                            it = nodes.find(succNode);
                            if (it == nodes.end() || inNested && it->second != nullptr)
                            {
                                generateNodeChanges();
                                goodPathFound = true;
                            }
                            else
                            {
                                if (searchNode(succNode, varChanges, tags, sefSucc))
                                {
                                    if (firstSearched) mergeMaps(varChanges.at(node), varChanges.at(succNode));
                                    else mergeMaps(varChanges.at(node), varChanges.at(succNode));
                                }
                            }
                        }
                        break;
                    }
                    default:
                        throw std::runtime_error("weird enum");
                }
            }
            else
            {
                long double slowestApproach;
                long double fastestApproach;
                bool moving = lhVar->getRelativeVelocity(rhVar.get(), slowestApproach, fastestApproach);

                switch (lhVar->canMeet(jocc->op, rhVar.get()))
                {
                    case SymbolicDouble::MeetEnum::MUST:
                    {
                        it = nodes.find(succNode);
                        if (it == nodes.end() || inNested && it->second != nullptr)
                        {
                            generateNodeChanges();
                            goodPathFound = true;
                        }
                        else
                        {
                            shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                    = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                            if (sefSucc->addPathCondition(node->getName(), jocc))
                            {
                                searchNode(succNode, varChanges, tags, sefSucc);
                                mergeMaps(varChanges.at(node), varChanges.at(succNode));
                            }
                        }
                    }

                    if (moving &&
                            (fastestApproach > 0 && (jocc->op == Relations::LT || jocc->op == Relations::LE))
                        ||  (slowestApproach < 0 && (jocc->op == Relations::GT || jocc->op == Relations::GE)))
                    {
                        it = nodes.find(failNode);
                        if (it == nodes.end() || inNested && it->second != nullptr)
                        {
                            generateNodeChanges();
                            goodPathFound = true;
                        }
                        else
                        {
                            shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFail
                                    = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                            short direction = jocc->op == Relations::LE || jocc->op == Relations::GE; //todo
                            jocc->term1.getVarWrapper()->getSymbolicDouble(sefFail.get())->iterateTo(rhVar.get());
                            if (sefFail->addPathCondition(node->getName(), jocc, true))
                            {
                                searchNode(failNode, varChanges, tags, sefFail);
                                mergeMaps(varChanges.at(node), varChanges.at(failNode));
                            }
                        }
                    }
                    break;

                    case SymbolicDouble::MeetEnum::CANT:
                    {
                        it = nodes.find(failNode);
                        if (it == nodes.end() || inNested && it->second != nullptr)
                        {
                            generateNodeChanges();
                            goodPathFound = true;
                        }
                        else
                        {
                            shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFail
                                    = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                            if (sefFail->addPathCondition(node->getName(), jocc, true))
                            {
                                searchNode(node->getCompFail(), varChanges, tags, sefFail);
                                mergeMaps(varChanges.at(node), varChanges.at(failNode));
                            }
                        }

                        if (moving &&
                            (fastestApproach > 0 && (jocc->op == Relations::LT || jocc->op == Relations::LE))
                            ||  (slowestApproach < 0 && (jocc->op == Relations::GT || jocc->op == Relations::GE)))
                        {
                            it = nodes.find(succNode);
                            if (it == nodes.end() || inNested && it->second != nullptr)
                            {
                                generateNodeChanges();
                                goodPathFound = true;
                            }
                            else
                            {
                                shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                        = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                                short direction = jocc->op == Relations::LE || jocc->op == Relations::GE;
                                jocc->term1.getVarWrapper()->getSymbolicDouble(sefSucc.get())->iterateTo(rhVar.get());
                                if (sefSucc->addPathCondition(node->getName(), jocc))
                                {
                                    searchNode(succNode, varChanges, tags, sefSucc);
                                    mergeMaps(varChanges.at(node), varChanges.at(succNode));
                                }
                            }
                        }
                    }
                    break;

                    case SymbolicDouble::MeetEnum::MAY:
                    {
                        it = nodes.find(succNode);
                        if (it == nodes.end() || inNested && it->second != nullptr)
                        {
                            generateNodeChanges();
                            goodPathFound = true;
                        }
                        else
                        {
                            shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefSucc
                                    = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                            if (sefSucc->addPathCondition(node->getName(), jocc)
                                && searchNode(node->getCompSuccess(), varChanges, tags, sefSucc))
                            {
                                mergeMaps(varChanges.at(node), varChanges.at(node->getCompSuccess()));
                            }
                        }

                        it = nodes.find(failNode);
                        if (it == nodes.end() || inNested && it->second != nullptr)
                        {
                            generateNodeChanges();
                            goodPathFound = true;
                        }
                        else
                        {
                            shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sefFail
                                    = make_shared<SymbolicExecution::SymbolicExecutionFringe>(sef);
                            if (sefFail->addPathCondition(node->getName(), jocc, true))
                            {
                                searchNode(node->getCompFail(), varChanges, tags, sefFail);
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
