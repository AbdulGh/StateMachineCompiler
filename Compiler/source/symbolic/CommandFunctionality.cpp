//
// Created by abdul on 10/03/18.
//

#include "../Command.h"
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
    if (toParse.empty()) throw "Can't parse an empty string";
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
            if (!indexWrapper) throw "went wrong";
            return make_unique<SDByIndexVar>(toParse.substr(0, first), move(indexWrapper));
        }
    }
}

//VarSetting superclass
WrapperHoldingCommand::WrapperHoldingCommand(std::unique_ptr<VarWrapper> vw, int linenum):
        AbstractCommand(linenum), vs(std::move(vw)) {}

void WrapperHoldingCommand::setVarWrapper(std::unique_ptr<VarWrapper> nvs) {vs = move(nvs);}

Atom::Atom(const string& s, bool allowEmpty): holding(false)
{
    if (allowEmpty && s.empty()) type = StringType::STRINGLIT;
    else type = getStringType(s);
    if (type == StringType::ID) throw runtime_error("bad");
    sptr = new string(s);
}

Atom::Atom(unique_ptr<VarWrapper> vg): holding(true)
{
    type = StringType::ID;
    vptr = vg.release();
}

Atom::~Atom()
{
    if (holding) delete vptr;
    else delete sptr;
}

Atom::Atom(const Atom& o): type(o.type)
{
    if (o.holding)
    {
        vptr = o.vptr->clone().release();
        holding = true;
    }
    else
    {
        sptr = new string(*o.sptr);
        holding = false;
    }
}

Atom::Atom(Atom&& o): type(o.type)
{
    if (o.holding)
    {
        vptr = o.vptr;
        o.vptr = nullptr;
        holding = true;
    }
    else
    {
        sptr = o.sptr;
        o.sptr = nullptr;
        holding = false;
    }
}

Atom& Atom::operator=(Atom&& o)
{
    type = o.type;
    if (o.holding)
    {
        vptr = o.vptr;
        o.vptr = nullptr;
        holding = true;
    }
    else
    {
        sptr = o.sptr;
        o.sptr = nullptr;
        holding = false;
    }
    return *this;
}

void Atom::swap(Atom& a)
{
    if (holding)
    {
        VarWrapper* vw = vptr;
        if (a.holding)
        {
            vptr = a.vptr;
            a.vptr = vw;
        }
        else
        {
            type = a.type;
            sptr = a.sptr;
            holding = false;
            a.type = StringType::ID;
            a.vptr = vw;
            a.holding = true;
        }
    }
    else
    {
        string* s = sptr;
        if (a.holding)
        {
            a.type = type;
            type = StringType::ID;
            holding = true;
            a.holding = false;
            vptr = a.vptr;
            a.sptr = s;
        }
        else
        {
            StringType t = type;
            type = a.type;
            a.type = t;
            sptr = a.sptr;
            a.sptr = s;
        }
    }
}

void Atom::become(const Atom& other)
{
    if (holding) delete vptr; else delete sptr;
    type = other.type;
    holding = other.holding;
    if (holding) vptr = other.vptr->clone().release();
    else sptr = new string(*other.sptr);
}

void Atom::set(const string& s)
{
    if (holding) delete vptr;
    else delete sptr;
    type = getStringType(s);
    if (type == StringType::ID) throw runtime_error("bad");
    sptr = new string(s);
    holding = false;
}

void Atom::set(unique_ptr<VarWrapper> vg)
{
    if (holding) delete vptr;
    else delete sptr;
    type = StringType::ID;
    vptr = vg.release();
    holding = true;
}

bool Atom::operator<(const Atom& right) const
{
    if (holding != right.holding) return holding;
    else if (holding) return vptr->getFullName() < right.vptr->getFullName();
    else return (*sptr) < *(right.sptr);
}

bool Atom::operator==(const Atom& right) const
{
    if (holding != right.holding) return false;
    else if (holding) return vptr->getFullName() == right.vptr->getFullName();
    else return (*sptr) == *(right.sptr);
}


bool Atom::isHolding() const {return holding;}

const std::string* Atom::getString() const //todo make this return string ref directly
{
    return sptr;
}

const VarWrapper* Atom::getVarWrapper() const
{
    return vptr;
}

StringType Atom::getType() const {return type;}

Atom::operator std::string() const
{
    if (holding) return vptr->getFullName();
    else return *sptr;
}

//JumpOnComparisonCommand
JumpOnComparisonCommand::JumpOnComparisonCommand(const string& st, unique_ptr<VarWrapper> t1,
                                                 unique_ptr<VarWrapper> t2, Relations::Relop o, int linenum)
    :StateHoldingCommand(st, linenum), term1(move(t1)), term2(move(t2))
{
    op = o;
    setType(CommandType::CONDJUMP);
}

JumpOnComparisonCommand::JumpOnComparisonCommand(const string& st, unique_ptr<VarWrapper> t1,
                                                 const string& t2, Relations::Relop o, int linenum)
    :StateHoldingCommand(st, linenum), term1(move(t1)), term2(t2)
{
    op = o;
    setType(CommandType::CONDJUMP);
}

JumpOnComparisonCommand::JumpOnComparisonCommand(const JumpOnComparisonCommand& jocc)
    :StateHoldingCommand(jocc.getState(), jocc.getLineNum()), term1(jocc.term1), term2(jocc.term2)
{
    op = jocc.op;
    setType(CommandType::CONDJUMP);
}

//EvaluateExpressionCommand
Term::Term(const string& toParse) : vg{}
{
    parse(toParse);
}

void Term::parse(const string& toParse)
{
    StringType st;
    auto up = parseAccess(toParse, &st);
    switch (st)
    {
        case StringType::DOUBLELIT:
            isLit = true;
            d = stod(toParse);
            break;
        case StringType::ID:
            isLit = false;
            vg = move(up);
            break;
        default:
            throw "not allowed";
    }
}

Term::Term(const Term& other) : vg{}
{
    isLit = other.isLit;
    if (isLit) d = other.d;
    else vg = other.vg->clone();
}

Term::Term(double doub) : vg{}
{
    isLit = true;
    d = doub;
}

Term::Term(std::unique_ptr<VarWrapper> itmoveit): vg(move(itmoveit)), isLit(false) {}

Term::Term(Term&& other) : vg{}
{
    isLit = other.isLit;
    if (isLit) d = other.d;
    else vg = move(other.vg);
}

bool Term::operator==(Term& o)
{
    if (isLit)
    {
        if (o.isLit) return d == o.d;
        else return false;
    }
    else
    {
        if (!o.isLit) return vg->getFullName() == o.vg->getFullName();
        else return false;
    }
}

Term::~Term() {if (!isLit) vg.reset();}

string Term::str() const
{
    if (isLit) return to_string(d);
    else return vg->getFullName();
}

EvaluateExprCommand::EvaluateExprCommand(unique_ptr<VarWrapper> lh, Term t1,
                                         ArithOp o, Term t2, int linenum):
        WrapperHoldingCommand(move(lh), linenum),  op(o), term1(move(t1)), term2(move(t2))
{
    setType(CommandType::EXPR);
}

EvaluateExprCommand::EvaluateExprCommand(const EvaluateExprCommand& o):
        WrapperHoldingCommand(o.vs->clone(), o.getLineNum()), term1(o.term1), term2(o.term2), op(o.op) {}

EvaluateExprCommand::~EvaluateExprCommand()
{
    term1.~Term();
    term2.~Term();
}

unique_ptr<AbstractCommand> EvaluateExprCommand::clone()
{
    return make_unique<EvaluateExprCommand>(*this);
}

string EvaluateExprCommand::translation(const string& delim) const
{
    return vs->getFullName() + " = " + term1.str() + ' ' + opEnumChars[op]  + ' ' + term2.str() + ";" + delim;
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
}

AssignVarCommand::AssignVarCommand(unique_ptr<VarWrapper> lh, const string& rh, int linenum):
        AbstractCommand(linenum), vs(move(lh)), atom(move(rh))
{
    setType(CommandType::ASSIGNVAR);
}

AssignVarCommand::AssignVarCommand(unique_ptr<VarWrapper> lh, Atom& rh, int linenum):
        AbstractCommand(linenum), vs(move(lh)), atom(move(rh))
{
    setType(CommandType::ASSIGNVAR);
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
PushCommand::PushCommand(std::unique_ptr<VarWrapper> in, int linenum):
        AbstractCommand(linenum), calledFunction(nullptr),pushedVars(0), stringType(StringType::ID)
{
    atom = make_unique<Atom>(move(in));
    setType(CommandType::PUSH);
}

std::unique_ptr<AbstractCommand> PushCommand::clone()
{
    if (pushesState()) return std::make_unique<PushCommand>(state, getLineNum(), calledFunction);
    else if (atom->isHolding()) return std::make_unique<PushCommand>(atom->getVarWrapper()->clone(), getLineNum());
    else return std::make_unique<PushCommand>(*(atom->getString()), getLineNum(), nullptr);
}

