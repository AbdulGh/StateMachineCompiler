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
        //check for const comparison
        if (jocc->term1Type != JumpOnComparisonCommand::ComparitorType::ID
            && jocc->term2Type != JumpOnComparisonCommand::ComparitorType::ID)
        {
            if (jocc->term1Type != jocc->term2Type)
            {
                sef->error(Reporter::TYPE, "Tried to compare literals of different types", jocc->getLineNum());
                return false;
            }

            else
            {
                sef->reporter.optimising(Reporter::USELESS_OP, "Constant comparison: '" + jocc->translation() + "'");

                //replace conditionals with true/false
                bool isTrue;
                if (jocc->term1Type == JumpOnComparisonCommand::ComparitorType::DOUBLELIT)
                {
                    double d1 = stod(jocc->term1);
                    double d2 = stod(jocc->term2);
                    isTrue = Relations::evaluateRelop<double>(d1, jocc->op, d2);
                }
                else isTrue = Relations::evaluateRelop<string>(jocc->term1, jocc->op, jocc->term2);

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
        else //actually have to do some work
        {
            Relations::Relop op = jocc->op;

            //note: JOCC constructor ensures that if there is a var there is a var on the LHS
            VarPointer LHS = sef->symbolicVarSet.findVar(jocc->term1);
            if (LHS == nullptr)
            {
                sef->error(Reporter::UNDECLARED_USE, "'" + jocc->term1 + "' used without being declared",
                           jocc->getLineNum());
                return false;
            }

            //check if we can meet the comparison - search if so
            if (jocc->term2Type == JumpOnComparisonCommand::ComparitorType::DOUBLELIT)
            {
                if (LHS->getType() != DOUBLE)
                {
                    sef->error(Reporter::TYPE, "'" + jocc->term1 + "' (type " + TypeEnumNames[LHS->getType()]
                                               + ")  compared to a double literal",
                               jocc->getLineNum());
                    return false;
                }

                shared_ptr<SymbolicVariableTemplate<double>> LHS =
                        static_pointer_cast<SymbolicVariableTemplate<double>>(LHS);

                switch (op)
                {
                    case Relations::EQ:


                }
            }
        }
    }
    else visitNode(sef, n->getCompFail()); //wrong, check return todo
}