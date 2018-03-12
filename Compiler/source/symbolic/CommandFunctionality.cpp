//
// Created by abdul on 10/03/18.
//

#include "../Command.h"
#include "SymbolicVarWrappers.h"

using namespace std;

//PrintIndirectCommand
PrintIndirectCommand::PrintIndirectCommand(unique_ptr<VarGetter> sdg, int linenum):
        AbstractCommand(linenum), toPrint(move(sdg)) {setType(CommandType::PRINT), setData(toPrint->getFullName());}

PrintIndirectCommand::~PrintIndirectCommand() {toPrint.reset();}

string PrintIndirectCommand::translation(const string& delim) const
{
    return "print " + toPrint->getFullName() + ";" + delim;
}

unique_ptr<AbstractCommand> PrintIndirectCommand::clone()
{
    return make_unique<PrintIndirectCommand>(toPrint->clone(), getLineNum());
}

//JumpOnComparisonCommand
JumpOnComparisonCommand::JumpOnComparisonCommand(const string& st, unique_ptr<VarGetter> t1,
                                                 unique_ptr<VarGetter> t2, Relations::Relop o, int linenum)
    :AbstractCommand(linenum)
{
    setData(st);
    term1.vptr = t1.release();
    term2.vptr = t2.release();
    op = o;
    //these are only used during symbolic execution
    term1Type = term2Type = StringType::ID;
    setType(CommandType::CONDJUMP);
}

JumpOnComparisonCommand::JumpOnComparisonCommand(const string& st, unique_ptr<VarGetter> t1,
                                                 const string& t2, Relations::Relop o, int linenum)
    :AbstractCommand(linenum)
{
    setData(st);
    term1.vptr = t1.release();
    term2.sptr = new string(t2);
    op = o;
    //these are only used during symbolic execution
    term1Type = StringType::ID;
    term2Type = getStringType(*term2.sptr);
    setType(CommandType::CONDJUMP);
}

JumpOnComparisonCommand::JumpOnComparisonCommand(const JumpOnComparisonCommand& jocc)
        :AbstractCommand(jocc.getLineNum())
{
    setData(jocc.getData());

    if (jocc.term1Type == StringType::ID)
    {
        term1Type = StringType::ID;
        term1.vptr = jocc.term1.vptr->clone().release(); //probably will be optimised?
    }
    else
    {
        term1Type = jocc.term1Type;
        term1.sptr = new string(jocc.term1);
    }

    if (jocc.term2Type == StringType::ID)
    {
        term2Type = StringType::ID;
        term2.vptr = jocc.term2.vptr->clone().release();
    }
    else
    {
        term2Type = jocc.term2Type;
        term2.sptr = new string(jocc.term2);
    }

    op = jocc.op;

    setType(CommandType::CONDJUMP);
}

void JumpOnComparisonCommand::resetTerm1()
{
    if (term1Type == StringType::ID) delete term1.vptr;
    else delete term1.sptr;
}

void JumpOnComparisonCommand::resetTerm2()
{
    if (term2Type == StringType::ID) delete term2.vptr;
    else delete term2.sptr;
}

string JumpOnComparisonCommand::t1str() const
{
    if (term1Type == StringType::ID) return term1.vptr->getFullName();
    else return *term1.sptr;
}

string JumpOnComparisonCommand::t2str() const
{
    if (term2Type == StringType::ID) return term2.vptr->getFullName();
    else return *term2.sptr;
}
