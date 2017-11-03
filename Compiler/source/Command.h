#ifndef COMMAND_H
#define COMMAND_H

#include <memory>

#include "compile/Token.h"

enum class CommandType{JUMP, CONDJUMP, DECLAREVAR, PUSH, POP, ASSIGNVAR, CHANGEVAR, EXPR, PRINT};

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
    virtual std::string translation(const std::string& delim = "\n") const = 0;
    AbstractCommand() {}
    AbstractCommand(int line): linenumber(line) {}
    virtual ~AbstractCommand() {}
    virtual std::unique_ptr<AbstractCommand> clone() = 0;

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
        catch (std::invalid_argument&)
        {
            return StringType::ID;
        }
    }
};

class PrintCommand: public AbstractCommand
{
public:
    PrintCommand(const std::string& info, int linenum) : AbstractCommand(linenum)
    {
        setData(info);
        setType(CommandType::PRINT);
    }

    std::string translation(const std::string& delim) const override {return "print " + getData() + ";" + delim;}

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<PrintCommand>(getData(), getLineNum());
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
    std::string translation(const std::string& delim) const override {return "return;" + delim;} //meta

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<ReturnCommand>(getLineNum());
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
    std::string translation(const std::string& delim) const override{return "jump " + getData() + ";" + delim;};

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<JumpCommand>(getData(), getLineNum());
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

    JumpOnComparisonCommand(const std::string& st, const std::string& t1, const std::string& t2, Relations::Relop o, int linenum) : AbstractCommand(linenum)
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

    void setTerm1(const std::string& t1)
    {
        term1 = t1;
        term1Type = getStringType(term1);
    }

    void setTerm2(const std::string& t2)
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

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<JumpOnComparisonCommand>(getData(), term1, term2, op, getLineNum());
    }

    std::string translation(const std::string& delim) const override {return "jumpif " + term1 + relEnumStrs[op] + term2 + " " + getData() + ";" + delim;}
    std::string negatedTranslation(const std::string& delim) const {return "jumpif " + term1 +
                relEnumStrs[Relations::negateRelop(op)] + term2 + " " + getData() + ";" + delim;}
    std::string condition(const std::string& delim) const {return term1 + relEnumStrs[op] + term2 + delim;}
    std::string negatedCondition(const std::string& delim) const {return term1 + relEnumStrs[Relations::negateRelop(op)] + term2 + delim;}
};

class InputVarCommand: public AbstractCommand
{
public:
    InputVarCommand(const std::string& assigning, int linenum) : AbstractCommand(linenum)
    {
        setData(assigning);
        setType(CommandType::CHANGEVAR);
    }
    std::string translation(const std::string& delim) const override{return "input " + getData() + ";" + delim;};

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<InputVarCommand>(getData(), getLineNum());
    }

    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs) override;
};

class FunctionSymbol;
class PushCommand: public AbstractCommand
{
public:
    enum PushType{PUSHSTR, PUSHSTATE};
    PushType pushType;
    FunctionSymbol* calledFunction;

    PushCommand(const std::string& in, int linenum, FunctionSymbol* cf = nullptr):
            AbstractCommand(linenum), calledFunction(cf),
            pushType(cf == nullptr ? PUSHSTR : PUSHSTATE)
    {
        setData(in);
        setType(CommandType::PUSH);
    }

    std::string translation(const std::string& delim) const override
    {
        if (pushType == PUSHSTATE) return "push state " + getData() + ";" + delim;
        else return "push " + getData() + ";" + delim;
    }

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<PushCommand>(getData(), getLineNum(), calledFunction);
    }

    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs) override;
};

class PopCommand: public AbstractCommand
{
public:
    PopCommand(const std::string& in, int linenum) : AbstractCommand(linenum)
    {
        setData(in);
        setType(CommandType::POP);
    }

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<PopCommand>(getData(), getLineNum());
    }

    bool isEmpty() const {return getData() == "";}

    std::string translation(const std::string& delim) const override {return isEmpty() ? "pop;" + delim : "pop " + getData() + ";" + delim;}
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs) override;
};


class AssignVarCommand: public AbstractCommand
{
public:
    std::string RHS;
    AssignVarCommand(const std::string& lh, const std::string& rh, int linenum) : AbstractCommand(linenum)
    {
        setData(lh);
        RHS = rh;
        setType(CommandType::ASSIGNVAR);
    }

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<AssignVarCommand>(getData(), RHS, getLineNum());
    }

    std::string translation(const std::string& delim) const override{return getData() + " = " + RHS + ";" + delim;}
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs) override;
};

class EvaluateExprCommand: public AbstractCommand
{
public:
    std::string term1;
    std::string term2;
    Op op;

    EvaluateExprCommand(const std::string& lh, std::string t1, Op o, std::string t2, int linenum) :
    AbstractCommand(linenum), op{o}, term1{std::move(t1)}, term2{std::move(t2)}
    {
        setData(lh);
        setType(CommandType::EXPR);
    }

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<EvaluateExprCommand>(getData(), term1, op, term2, getLineNum());
    }

    std::string translation(const std::string& delim) const override{return getData() + " = " + term1 + ' '
                                                                     + opEnumChars[op]  + ' ' + term2 + ";" + delim;}
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs) override;
};

class DeclareVarCommand: public AbstractCommand
{
public:
    VariableType vt;

    DeclareVarCommand(VariableType t, const std::string& n, int linenum) : AbstractCommand(linenum), vt(t)
    {
        setData(n);
        setType(CommandType::DECLAREVAR);
    }

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<DeclareVarCommand>(vt, getData(), getLineNum());
    }

    std::string translation(const std::string& delim) const override
    {
        return VariableTypeEnumNames[vt] + " " + getData() + ";" + delim;
    }
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs) override;
};

#endif