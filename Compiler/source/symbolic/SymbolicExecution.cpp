#include "SymbolicExecution.h"

using namespace std;
using namespace SymbolicExecution;

/*SymbolicExecutionFringe*/
void SymbolicExecutionFringe::error(Reporter::AlertType a, std::string s, int linenum)
{
    if (linenum != -1) s += " (line " + std::to_string(linenum) + ")";
    reporter.error(a, s);
    feasable = false;
}

void SymbolicExecutionFringe::warn(Reporter::AlertType a, std::string s, int linenum)
{
    if (linenum != -1) s += " (line " + std::to_string(linenum) + ")";
    reporter.warn(a, s);
}

bool SymbolicExecutionFringe::hasSeen(std::string state)
{
    return pathConditions.find(state) != pathConditions.end();
}

bool SymbolicExecutionFringe::isFeasable()
{
    if (!symbolicVarSet.isFeasable()) feasable = false;
    return feasable;
}

/*SymbolicExecutionManager*/
void SymbolicExecutionManager::search()
{
    SymbolicExecutionFringe* sef = new SymbolicExecutionFringe(reporter);
    if (!visitNode(sef, cfg.getFirst()))
    {
        reporter.warn(Reporter::GENERIC, "No feasable path was found through the program");
    }
}

bool SymbolicExecutionManager::visitNode(SymbolicExecutionFringe* sef, shared_ptr<CFGNode> n)
{
    if (!sef->isFeasable()) return false;
    else if (sef->hasSeen(n->getName())) return true;

    for (shared_ptr<AbstractCommand> command : n->getInstrs())
    {
        if (!command->acceptSymbolicExecution(sef)) return false;
    }

    feasableVisits[n->getName()]++;
    if (n->getName() == cfg.getLast()->getName()) return true;

    shared_ptr<JumpOnComparisonCommand> jocc = n->getComp();
    if (jocc != nullptr) //is a conditional jump
    {
        shared_ptr<SymbolicVariable> t1 = sef->symbolicVarSet.findVar(jocc->term1);
        if (t1 == nullptr)
        {
            //shouldn't happen
            return false;
        }

    }
}