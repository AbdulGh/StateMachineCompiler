#include "SymbolicExecution.h"

using namespace std;
using namespace SymbolicExecution;

/*SymbolicExecutionFringe*/
SymbolicExecutionFringe::SymbolicExecutionFringe(Reporter &r, SymbolicExecutionFringe* p) :
        reporter(r),
        parent(p),
        currentStack(parent->currentStack),
        symbolicVarSet(parent->symbolicVarSet){}


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
        if (jocc->term1Type != VAR && jocc->term1Type == jocc->term2Type)
        {
            sef->reporter.optimising(Reporter::USELESS_OP, "Constant comparison: '" + jocc->translation() + "'");

            //replace conditionals with true/false
            bool isTrue;
            if (jocc->term1Type == DOUBLE)
            {
                double d1 = stod(jocc->term1);
                double d2 = stod(jocc->term2);
                isTrue = evaluateRelop<double>(d1, jocc->op, d2);
            }
            else isTrue = evaluateRelop<string>(jocc->term1, jocc->op, jocc->term2);

            if (isTrue)
            {
                n->getCompFail()->removeParent(n);
                n->setCompFail(n->getCompSuccess());
            }
            else n->getCompSuccess()->removeParent(n);
            n->getComp().reset();
            n->setComp(nullptr);
        }
    }
    else //todo continue here by actually doing the bounding! and fix comparison confusion (VarType and ComparitorType)
    {

    }
}