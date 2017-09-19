#ifndef COMMAND_H
#define COMMAND_H

#include <memory>

#include "compile/Token.h"

enum class CommandType{NONE, JUMP, CONDJUMP, DECLAREVAR, PUSH, POP, ASSIGNVAR, CHANGEVAR, EXPR};

namespace SymbolicExecution {class SymbolicExecutionFringe;}; //symbolic/SymbolicExecution.cpp
class AbstractCommand
{
private:
    std::string data;
    CommandType commandType;
    int linenumber;

protected:
    void setType(CommandType type)
    {
        AbstractCommand::commandType = type;
    }

public:
    virtual std::string translation() const = 0;
    AbstractCommand() {}
    AbstractCommand(int line): linenumber(line) {}
    virtual ~AbstractCommand() {}
    virtual std::shared_ptr<AbstractCommand> clone() = 0;

    //returns if the symbolic execution of this command went through
    virtual bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs);

    const std::string& getData() const
    {
        return data;
    }

    CommandType getType() const
    {
        return commandType;
    }

    void setData(const std::string &data)
    {
        AbstractCommand::data = data;
    }

    int getLineNum()
    {
        return linenumber;
    }

    enum class StringType{ID, STRINGLIT, DOUBLELIT};
    static AbstractCommand::StringType getStringType(const std::string& str)
    {
        if (str.length() == 0) throw std::runtime_error("Asked to find StringType of empty string");
        else if (str.length() > 1 && str[0] == '\"') return StringType::STRINGLIT;
        else try
        {
            stod(str);
            return StringType::DOUBLELIT;
        }
        catch (std::invalid_argument e)
        {
            return StringType::ID;
        }
    }
};

class PrintCommand: public AbstractCommand
{
public:
    PrintCommand(std::string info, int linenum) : AbstractCommand(linenum)
    {
        setData(info);
        setType(CommandType::NONE);
    }

    std::string translation() const override {return "print " + getData() + ";\n";}

    std::shared_ptr<AbstractCommand> clone() override
    {
        return std::make_shared<PrintCommand>(getData(), getLineNum());
    }
};

class ReturnCommand: public AbstractCommand
{
public:
    ReturnCommand(int linenum) : AbstractCommand(linenum)
    {
        setData("return");
        setType(CommandType::JUMP);
    }
    std::string translation() const override {return "return;\n";} //meta

    std::shared_ptr<AbstractCommand> clone() override
    {
        return std::make_shared<ReturnCommand>(getLineNum());
    }
};

class JumpCommand: public AbstractCommand
{
public:
    JumpCommand(std::string to, int linenum) : AbstractCommand(linenum)
    {
        setData(to);
        setType(CommandType::JUMP);
    }
    std::string translation() const override{return "jump " + getData() + ";\n";};

    std::shared_ptr<AbstractCommand> clone() override
    {
        return std::make_shared<JumpCommand>(getData(), getLineNum());
    }
};

class JumpOnComparisonCommand: public AbstractCommand
{
public:
    std::string term1;
    StringType term1Type;
    std::string term2;
    StringType term2Type;
    Relations::Relop op;

    JumpOnComparisonCommand(std::string st, std::string t1, std::string t2, Relations::Relop o, int linenum) : AbstractCommand(linenum)
    {
        setData(st);
        term1 = t1;
        term2 = t2;
        op = o;
        //these are only used during symbolic execution
        term1Type = getStringType(term1);
        term2Type = getStringType(term2);

        //ensure a var appears on the lhs if there is one
        //const comparisons will be hardcoded later
        if (term1Type != AbstractCommand::StringType::ID
            && term2Type == AbstractCommand::StringType::ID)
        {
            StringType temp = term1Type;
            term1Type = term2Type;
            term2Type = temp;
            term1.swap(term2);
            op = Relations::mirrorRelop(op);
        }

        setType(CommandType::CONDJUMP);
    }

    void setTerm1(std::string t1)
    {
        term1 = t1;
        term1Type = getStringType(term1);
    }

    void setTerm2(std::string t2)
    {
        term2 = t2;
        term2Type = getStringType(term2);
    }

    void makeGood()
    {
        if (term1Type != AbstractCommand::StringType::ID
            && term2Type == AbstractCommand::StringType::ID)
        {
            StringType temp = term1Type;
            term1Type = term2Type;
            term2Type = temp;
            term1.swap(term2);
            op = Relations::mirrorRelop(op);
        }
    }

    std::shared_ptr<AbstractCommand> clone() override
    {
        return std::make_shared<JumpOnComparisonCommand>(getData(), term1, term2, op, getLineNum());
    }

    std::string translation() const override {return "jumpif " + term1 + relEnumStrs[op] + term2 + " " + getData() + ";\n";}
};

class InputVarCommand: public AbstractCommand
{
public:
    InputVarCommand(const std::string& assigning, int linenum) : AbstractCommand(linenum)
    {
        setData(assigning);
        setType(CommandType::CHANGEVAR);
    }
    std::string translation() const override{return "input " + getData() + ";\n";};

    std::shared_ptr<AbstractCommand> clone() override
    {
        return std::make_shared<InputVarCommand>(getData(), getLineNum());
    }

    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs) override;
};

class PushCommand: public AbstractCommand
{
public:
    enum PushType{PUSHVAR, PUSHSTATE};
    PushType pushType;

    PushCommand(std::string in, int linenum) : AbstractCommand(linenum)
    {
        if (in.find("state") == 0)
        {
            pushType = PUSHSTATE;
            setData(in.substr(5));
        }
        else
        {
            pushType = PUSHVAR;
            setData(in);
        }
        setType(CommandType::PUSH);
    }

    std::string translation() const override{return "push " + getData() + ";\n";}

    std::shared_ptr<AbstractCommand> clone() override
    {
        return std::make_shared<PushCommand>(getData(), getLineNum());
    }

    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs) override;
};

class PopCommand: public AbstractCommand
{
public:
    PopCommand(std::string in, int linenum) : AbstractCommand(linenum)
    {
        setData(in);
        setType(CommandType::POP);
    }

    std::shared_ptr<AbstractCommand> clone() override
    {
        return std::make_shared<PopCommand>(getData(), getLineNum());
    }

    std::string translation() const override{return "pop " + getData() + ";\n";}
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs) override;
};


class AssignVarCommand: public AbstractCommand
{
public:
    std::string RHS;
    AssignVarCommand(std::string lh, std::string rh, int linenum) : AbstractCommand(linenum)
    {
        setData(lh);
        RHS = rh;
        setType(CommandType::ASSIGNVAR);
    }

    std::shared_ptr<AbstractCommand> clone() override
    {
        return std::make_shared<AssignVarCommand>(getData(), RHS, getLineNum());
    }

    std::string translation() const override{return getData() + " = " + RHS + ";\n";}
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs) override;
};

class EvaluateExprCommand: public AbstractCommand
{
public:
    std::string term1;
    std::string term2;
    Op op;

    EvaluateExprCommand(std::string lh, std::string t1, Op o, std::string t2, int linenum) : AbstractCommand(linenum),
                                                                                             op{o}, term1{t1}, term2{t2}
    {
        setData(lh);
        setType(CommandType::EXPR);
    }

    std::shared_ptr<AbstractCommand> clone() override
    {
        return std::make_shared<EvaluateExprCommand>(getData(), term1, op, term2, getLineNum());
    }

    std::string translation() const override{return getData() + " = " + term1 + ' ' + opEnumChars[op]  + ' ' + term2 + ";\n";}
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs) override;
};

class DeclareVarCommand: public AbstractCommand
{
public:
    VariableType vt;

    DeclareVarCommand(VariableType t, std::string n, int linenum) : AbstractCommand(linenum), vt(t)
    {
        setData(n);
        setType(CommandType::DECLAREVAR);
    }

    std::shared_ptr<AbstractCommand> clone() override
    {
        return std::make_shared<DeclareVarCommand>(vt, getData(), getLineNum());
    }

    std::string translation() const override
    {
        std::string debug = VariableTypeEnumNames[vt] + " " + getData() + ";\n";
        return VariableTypeEnumNames[vt] + " " + getData() + ";\n";
    }
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs) override;
};

#endif