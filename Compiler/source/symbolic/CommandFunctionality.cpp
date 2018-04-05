//
// Created by abdul on 10/03/18.
//

#include "../Command.h"
#include "SymbolicDouble.h"
#include "VarWrappers.h"

using namespace std;

StringType getStringType(const std::string& str)
{
    if (str.length() == 0) throw std::runtime_error("Asked to find StringType of empty string");
    else if (str.length() > 1 && str[0] == '\"') return StringType::STRINGLIT;
    else
    try
    {
        stod(str);
        return StringType::DOUBLELIT;
    }
    catch (std::invalid_argument&)
    {
        return StringType::ID;
    }
}

unique_ptr<VarWrapper> parseAccess(const string& toParse, StringType* st = nullptr)
{
    if (toParse.empty()) throw std::runtime_error("Can't parse an empty string");
    if (toParse[0] == '\"')
    {
        if (st) *st = StringType::STRINGLIT;
        return nullptr;
    }
    else try
    {
        stod(toParse);
        if (st) *st = StringType::DOUBLELIT;
        return nullptr;
    }
    catch (invalid_argument&) //id or array access
    {
        if (st) *st = StringType::ID;
        size_t first = toParse.find('[');
        if (first == -1) return make_unique<SVByName>(toParse);
        string index = toParse.substr(first + 1, toParse.size() - first - 2);
        try
        {
            double dAttempt = stod(index);
            return make_unique<SDByArrayIndex>(toParse.substr(0, first), dAttempt);
        }
        catch (invalid_argument&)
        {
            unique_ptr<VarWrapper> indexWrapper = parseAccess(index);
            if (!indexWrapper) throw std::runtime_error("went wrong");
            return make_unique<SDByIndexVar>(toParse.substr(0, first), move(indexWrapper));
        }
    }
}

//VarSetting superclasses
WrapperHoldingCommand::WrapperHoldingCommand(std::unique_ptr<VarWrapper> vw, int linenum):
        AbstractCommand(linenum), vs(std::move(vw)) {}

StringHoldingCommand::~StringHoldingCommand() = default;
AtomHoldingCommand::~AtomHoldingCommand() = default;
WrapperHoldingCommand::~WrapperHoldingCommand() = default;

void WrapperHoldingCommand::setVarWrapper(std::unique_ptr<VarWrapper> nvs) {vs = move(nvs);}

//Atom
Atom::Atom(double nd):
    holding(false), type(StringType::DOUBLELIT), d(nd) {}

Atom::Atom() = default;
Atom::~Atom() = default;

Atom::Atom(unique_ptr<VarWrapper> vg): holding(true)
{
    type = StringType::ID;
    vptr = move(vg);
}

Atom::Atom(const Atom& o): type(o.type)
{
    if (o.holding)
    {
        vptr = o.vptr->clone();
        holding = true;
    }
    else
    {
        d = o.d;
        holding = false;
    }
}

Atom::Atom(Atom&& o): type(o.type)
{
    if (o.holding)
    {
        vptr = move(o.vptr);
        holding = true;
        type = StringType::ID;
    }
    else
    {
        d = o.d;
        holding = false;
        type = StringType::DOUBLELIT;
    }
    o.vptr = nullptr;
}

Atom& Atom::operator=(const Atom& o)
{
    type = o.type;
    if (o.holding)
    {
        vptr = o.vptr->clone();
        holding = true;
    }
    else
    {
        d = o.d;
        holding = false;
    }
    return *this;
}

Atom& Atom::operator=(Atom&& o)
{
    if (o.holding)
    {
        vptr = move(o.vptr);
        o.vptr = nullptr;
        holding = true;
        type = StringType::ID;
    }
    else
    {
        d = o.d;
        holding = false;
        type = StringType::DOUBLELIT;
    }
    o.vptr = nullptr;
    return *this;
}

void Atom::swap(Atom& a)
{
    if (holding)
    {
        unique_ptr<VarWrapper> vw = move(vptr);
        if (a.holding)
        {
            vptr = move(a.vptr);
            a.vptr = move(vw);
        }
        else
        {
            type = a.type;
            d = a.d;
            holding = false;
            a.type = StringType::ID;
            a.vptr = move(vw);
            a.holding = true;
        }
    }
    else
    {
        double t = d;
        if (a.holding)
        {
            a.type = type;
            type = StringType::ID;
            holding = true;
            a.holding = false;
            vptr = move(a.vptr);
            a.d = t;
        }
        else
        {
            StringType ttype = type;
            type = a.type;
            a.type = ttype;
            d = a.d;
            a.d = t;
        }
    }
}

void Atom::become(const Atom& other)
{
    type = other.type;
    holding = other.holding;
    if (holding) vptr = other.vptr->clone();
    else d = other.d;
}

void Atom::set(double nd)
{
    type = StringType::DOUBLELIT;
    d = nd;
    holding = false;
}

void Atom::set(unique_ptr<VarWrapper> vg)
{
    type = StringType::ID;
    vptr = move(vg);
    holding = true;
}

bool Atom::operator<(const Atom& right) const
{
    if (holding != right.holding) return holding;
    else if (holding) return vptr->getFullName() < right.vptr->getFullName();
    else return d < right.d;
}

bool Atom::operator==(const Atom& right) const
{
    if (holding != right.holding) return false;
    else if (holding) return vptr->getFullName() == right.vptr->getFullName();
    else return d == right.d;
}

bool Atom::isHolding() const {return holding;}

double Atom::getLiteral() const
{
    return d;
}
VarWrapper* Atom::getVarWrapper() const
{
    return vptr.get();
}

StringType Atom::getType() const {return type;}

Atom::operator std::string() const
{
    if (holding) return vptr->getFullName();
    else return to_string(d);
}

void Atom::resetRepeatBounds(SymbolicExecution::SymbolicExecutionFringe* sef)
{
    if (!isHolding()) throw runtime_error("not holding unique ptr");
    GottenVarPtr<SymbolicDouble> gsv = vptr->getSymbolicDouble(sef);
    gsv->resetRepeatBounds();
    if (gsv.constructed()) vptr->setSymbolicDouble(sef, gsv.get());
}

//JumpOnComparisonCommand
JumpOnComparisonCommand::JumpOnComparisonCommand(const string& st, unique_ptr<VarWrapper> t1,
                                                 unique_ptr<VarWrapper> t2, Relations::Relop o, int linenum)
    :StringHoldingCommand(st, linenum), term1(move(t1)), term2(move(t2))
{
    op = o;
    setType(CommandType::CONDJUMP);
}

JumpOnComparisonCommand::JumpOnComparisonCommand(const string& st, unique_ptr<VarWrapper> t1,
                                                 double t2, Relations::Relop o, int linenum)
    :StringHoldingCommand(st, linenum), term1(move(t1)), term2(t2)
{
    op = o;
    setType(CommandType::CONDJUMP);
}

JumpOnComparisonCommand::JumpOnComparisonCommand(const JumpOnComparisonCommand& jocc)
    :StringHoldingCommand(jocc.getString(), jocc.getLineNum()), term1(jocc.term1), term2(jocc.term2)
{
    op = jocc.op;
    setType(CommandType::CONDJUMP);
}

EvaluateExprCommand::EvaluateExprCommand(unique_ptr<VarWrapper> lh, Atom t1,
                                         ArithOp o, Atom t2, int linenum):
        WrapperHoldingCommand(move(lh), linenum),  op(o), term1(move(t1)), term2(move(t2))
{
    setType(CommandType::EXPR);
}

EvaluateExprCommand::EvaluateExprCommand(const EvaluateExprCommand& o):
        WrapperHoldingCommand(o.vs->clone(), o.getLineNum()), term1(o.term1), term2(o.term2), op(o.op) {}

unique_ptr<AbstractCommand> EvaluateExprCommand::clone()
{
    return make_unique<EvaluateExprCommand>(*this);
}

string EvaluateExprCommand::translation(const string& delim) const
{
    return vs->getFullName() + " = " + string(term1) + ' ' + opEnumChars[op]  + ' ' + string(term2) + ";" + delim;
}

//InputVarCommand
InputVarCommand::InputVarCommand(unique_ptr<VarWrapper> into, int linenum) : WrapperHoldingCommand(move(into), linenum)
{
    setType(CommandType::INPUTVAR);
}

string InputVarCommand::translation(const string& delim) const
{
    return "input " + vs->getFullName() + ";" + delim;
};

unique_ptr<AbstractCommand> InputVarCommand::clone()
{
    return make_unique<InputVarCommand>(vs->clone(), getLineNum());
}

//AssignVarCommand
AssignVarCommand::AssignVarCommand(unique_ptr<VarWrapper> lh, unique_ptr<VarWrapper> rh, int linenum):
        AbstractCommand(linenum), vs(move(lh)), atom(move(rh))
{
    setType(CommandType::ASSIGNVAR);
    if (vs->getFullName() == "LHS" && string(atom) == "_3_0_y")
    {
        int debug;
        debug = 2;
    }
}

AssignVarCommand::AssignVarCommand(unique_ptr<VarWrapper> lh, Atom rh, int linenum):
        AbstractCommand(linenum), vs(move(lh)), atom(move(rh))
{
    setType(CommandType::ASSIGNVAR);
    if (vs->getFullName() == "LHS" && string(atom) == "_3_0_y")
    {
        int debug;
        debug = 2;
    }
}

void AssignVarCommand::setVarWrapper(std::unique_ptr<VarWrapper> sv)
{
    vs = move(sv);
}

string AssignVarCommand::translation(const string& delim) const
{
    return vs->getFullName() + " = " + string(atom) + ";" + delim;
}

unique_ptr<AbstractCommand> AssignVarCommand::clone()
{
    return make_unique<AssignVarCommand>(vs->clone(), atom, getLineNum());
}

//PopCommand
PopCommand::PopCommand(unique_ptr<VarWrapper> into, int linenum): WrapperHoldingCommand(move(into), linenum)
{
    setType(CommandType::POP);
}

void PopCommand::clear()
{
    vs.reset();
    vs = nullptr;
}

string PopCommand::translation(const string& delim) const
{
    return isEmpty() ? "pop;" + delim : "pop " + vs->getFullName() + ";" + delim;
}

unique_ptr<AbstractCommand> PopCommand::clone()
{
    return make_unique<PopCommand>(vs->clone(), getLineNum());
}

//PushCommand
PushCommand::PushCommand(Atom in, int linenum):
        AbstractCommand(linenum), calledFunction(nullptr),pushedVars(0)
{
    atom = move(in);
    setType(CommandType::PUSH);
}

std::string PushCommand::translation(const std::string& delim) const
{
    if (pushesState()) return "push state " + s + ";" + delim;
    else if (!atom.isHolding()) return "push " + to_string(atom.getLiteral()) + ";" + delim;
    else return "push " + atom.getVarWrapper()->getFullName() + ";" + delim;
}

std::unique_ptr<AbstractCommand> PushCommand::clone()
{
    if (pushesState()) return std::make_unique<PushCommand>(s, getLineNum(), calledFunction);
    else return std::make_unique<PushCommand>(atom, getLineNum());
}

