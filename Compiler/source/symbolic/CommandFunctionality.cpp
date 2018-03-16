//
// Created by abdul on 10/03/18.
//

#include "../Command.h"
#include "../compile/VarWrappers.h"

using namespace std;

std::unique_ptr<VarGetter> parseAccess(const std::string& toParse, StringType* st = nullptr)
{
    if (toParse.empty()) throw "Can't parse an empty string";
    if (toParse[0] == '\"')
    {
        if (st) *st = StringType::STRINGLIT;
        return nullptr;
    }
    else try
    {
        std::stod(toParse);
        if (st) *st = StringType::DOUBLELIT;
        return nullptr;
    }
    catch (std::invalid_argument&) //id or array access
    {
        if (st) *st = StringType::ID;
        size_t first = toParse.find("[");
        if (first == -1) return std::make_unique<GetSVByName>(toParse);
        std::string index = toParse.substr(first + 1, toParse.size() - first - 2);
        try
        {
            double dAttempt = stod(index);
            return std::make_unique<GetSDByArrayIndex>(toParse.substr(0, first), dAttempt);
        }
        catch (std::invalid_argument&)
        {
            std::unique_ptr<VarGetter> indexWrapper = parseAccess(index);
            if (!indexWrapper) throw "went wrong";
            return std::make_unique<GetSDByIndexVar>(toParse.substr(0, first), move(indexWrapper));
        }
    }
}

Atom::Atom(const string& s)
{
    set(s);
}

Atom::Atom(unique_ptr<VarGetter> vg)
{
    set(move(vg));
}

Atom::~Atom()
{
    if (type == StringType::ID) delete vptr;
    else delete sptr;
}

Atom::Atom(const Atom& o): type(o.type)
{
    if (type == StringType::ID) vptr = o.vptr->clone().release();
    else sptr = new string(*o.sptr);
}

void Atom::set(const string& s)
{
    if (type == StringType::ID) delete vptr;
    else delete sptr;
    type = AbstractCommand::getStringType(s);
    if (type == StringType::ID) throw "ow!";
    sptr = new string(s);
}

void Atom::set(unique_ptr<VarGetter> vg)
{
    if (type == StringType::ID) delete vptr;
    else delete sptr;
    type = StringType::ID;
    vptr = vg.release();
}

Atom::operator std::string() const
{
    if (type == StringType::ID) return vptr->getFullName();
    else return *sptr;
}

//JumpOnComparisonCommand
JumpOnComparisonCommand::JumpOnComparisonCommand(const string& st, unique_ptr<VarGetter> t1,
                                                 unique_ptr<VarGetter> t2, Relations::Relop o, int linenum)
    :AbstractCommand(linenum), term1(move(t1)), term2(move(t2))
{
    setState(st);
    op = o;
    setType(CommandType::CONDJUMP);
}

JumpOnComparisonCommand::JumpOnComparisonCommand(const string& st, unique_ptr<VarGetter> t1,
                                                 const string& t2, Relations::Relop o, int linenum)
    :AbstractCommand(linenum), term1(move(t1)), term2(t2)
{
    setState(st);
    op = o;
    setType(CommandType::CONDJUMP);
}

JumpOnComparisonCommand::JumpOnComparisonCommand(const JumpOnComparisonCommand& jocc)
        :AbstractCommand(jocc.getLineNum()), term1(jocc.term1), term2(jocc.term2)
{
    setState(jocc.getState());

    if (jocc.term1.type == StringType::ID)
    {
        term1.type = StringType::ID;
        term1.vptr = jocc.term1.vptr->clone().release(); //probably will be optimised?
    }
    else
    {
        term1.type = jocc.term1.type;
        term1.sptr = new string(*jocc.term1.sptr);
    }

    if (jocc.term2.type == StringType::ID)
    {
        term2.type = StringType::ID;
        term2.vptr = jocc.term2.vptr->clone().release();
    }
    else
    {
        term2.type = jocc.term2.type;
        term2.sptr = new string(*jocc.term2.sptr);
    }

    op = jocc.op;

    setType(CommandType::CONDJUMP);
}

/*
void JumpOnComparisonCommand::resetTerm1()
{
    if (term1.type == StringType::ID) delete term1.vptr;
    else delete term1.sptr;
}

void JumpOnComparisonCommand::resetTerm2()
{
    if (term2.type == StringType::ID) delete term2.vptr;
    else delete term2.sptr;
}

string JumpOnComparisonCommand::t1str() const
{
    if (term1.type == StringType::ID) return term1.vptr->getFullName();
    else return *term1.sptr;
}

string JumpOnComparisonCommand::t2str() const
{
    if (term2.type == StringType::ID) return term2.vptr->getFullName();
    else return *term2.sptr;
}*/

//EvaluateExpressionCommand
EvaluateExprCommand::Term::Term(const std::string& toParse) : vg{}
{
    parse(toParse);
}

void EvaluateExprCommand::Term::parse(const std::string& toParse)
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

EvaluateExprCommand::Term::Term(const Term& other) : vg{}
{
    isLit = other.isLit;
    if (isLit) d = other.d;
    else vg = other.vg->clone();
}

EvaluateExprCommand::Term::Term(Term&& other) : vg{}
{
    isLit = other.isLit;
    if (isLit) d = other.d;
    else vg = move(other.vg);
}

bool EvaluateExprCommand::Term::operator==(Term& o)
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

EvaluateExprCommand::Term::~Term() {if (!isLit) vg.reset();}

std::string EvaluateExprCommand::Term::str() const
{
    if (isLit) return to_string(d);
    else return vg->getFullName();
}

EvaluateExprCommand::EvaluateExprCommand(std::unique_ptr<VarSetter> lh, Term t1,
                                         ArithOp o, Term t2, int linenum):
        AbstractCommand(linenum), lhs(move(lh)), op{o}, term1{move(t1)}, term2{move(t2)} //todo br
{
    setType(CommandType::EXPR);
}

EvaluateExprCommand::EvaluateExprCommand(const EvaluateExprCommand& o):
    lhs(o.lhs->clone()), term1(o.term1), term2(o.term2), op(o.op) {}

EvaluateExprCommand::~EvaluateExprCommand()
{
    term1.~Term();
    term2.~Term();
}

std::unique_ptr<AbstractCommand> EvaluateExprCommand::clone()
{
    return std::make_unique<EvaluateExprCommand>(*this);
}

std::string EvaluateExprCommand::translation(const std::string& delim) const
{
    return lhs->getFullName() + " = " + term1.str() + ' ' + opEnumChars[op]  + ' ' + term2.str() + ";" + delim;
}

//InputVarCommand
InputVarCommand::InputVarCommand(std::unique_ptr<VarSetter> into, int linenum) : AbstractCommand(linenum), vs(move(into))
{
    setType(CommandType::INPUTVAR);
}

std::string InputVarCommand::translation(const std::string& delim) const
{
    return "input " + vs->getFullName() + ";" + delim;
};

std::unique_ptr<AbstractCommand> InputVarCommand::clone()
{
    return std::make_unique<InputVarCommand>(vs->clone(), getLineNum());
}

//AssignVarCommand
AssignVarCommand::AssignVarCommand(std::unique_ptr<VarSetter> lh, std::unique_ptr<VarGetter> rh, int linenum):
        AbstractCommand(linenum), rhs(move(rh)), lhs(move(lh))
{
    setType(CommandType::ASSIGNVAR);
}

AssignVarCommand::AssignVarCommand(std::unique_ptr<VarSetter> lh, const std::string& rh, int linenum):
    AbstractCommand(linenum), rhs(rh), lhs(move(lh))
{
    setType(CommandType::ASSIGNVAR);
}

std::string AssignVarCommand::translation(const std::string& delim) const
{
    return lhs->getFullName() + " = " + string(rhs) + ";" + delim;
}

unique_ptr<AbstractCommand> AssignVarCommand::clone()
{
    return std::make_unique<AssignVarCommand>(lhs->clone(), rhs, getLineNum());
}

//PopCommand
PopCommand::PopCommand(std::unique_ptr<VarSetter> into, int linenum): AbstractCommand(linenum), vs(move(into))
{
    setType(CommandType::POP);
}

std::string PopCommand::translation(const std::string& delim) const
{
    return isEmpty() ? "pop;" + delim : "pop " + vs->getFullName() + ";" + delim;
}

std::unique_ptr<AbstractCommand> PopCommand::clone()
{
    return std::make_unique<PopCommand>(vs->clone(), getLineNum());
}

