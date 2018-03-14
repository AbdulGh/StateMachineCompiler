#ifndef COMMAND_H
#define COMMAND_H

#include <memory>

#include "compile/Token.h"

class VarWrapper;
class VarGetter;
class VarSetter;

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

    //returns true if the symbolic execution of this command went through
    virtual bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat = false);

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

    int getLineNum() const
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

class Atom
{
public:
    union
    {
        std::string* sptr;
        VarGetter* vptr;
    };

    Atom(const std::string&);
    Atom(std::unique_ptr<VarGetter>);
    Atom(const Atom& o);
    void set(const std::string& sptr);
    void set(std::unique_ptr<VarGetter> sptr);
    AbstractCommand::StringType type;
    operator std::string() const;
    ~Atom();
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

class PrintIndirectCommand: public AbstractCommand //done in CommandAcceptSymbolicExecution (forward declarations)
{
    std::unique_ptr<VarGetter> toPrint;

public:
    PrintIndirectCommand(std::unique_ptr<VarGetter> sdg, int linenum);
    ~PrintIndirectCommand() override;
    std::string translation(const std::string& delim) const override;
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat) override;
    std::unique_ptr<AbstractCommand> clone() override;

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
    JumpCommand(const std::string& to, int linenum) : AbstractCommand(linenum)
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
    Atom term1;
    Atom term2;
    
    Relations::Relop op;

    JumpOnComparisonCommand(const std::string& st, std::unique_ptr<VarGetter> t1,
                            std::unique_ptr<VarGetter> t2, Relations::Relop o, int linenum);

    JumpOnComparisonCommand(const std::string& st, std::unique_ptr<VarGetter> t1,
                            const std::string& t2, Relations::Relop o, int linenum);

    JumpOnComparisonCommand(const JumpOnComparisonCommand& jocc);
    
    void makeGood()
    {
        if (term1.type != AbstractCommand::StringType::ID
            && term2.type == AbstractCommand::StringType::ID)
        {
            auto tempptr = term2.vptr;
            if (term1.type == StringType::ID) term2.vptr = term1.vptr;
            else term2.sptr = term1.sptr;
            term2.type = term1.type;
            term1.type = StringType::ID;
            op = Relations::mirrorRelop(op);
        }
    }

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<JumpOnComparisonCommand>(*this);
    }

    std::string translation(const std::string& delim) const override 
    {
        return "jumpif " + std::string(term1) + relEnumStrs[op] + std::string(term2) + " " + getData() + ";" + delim;
    }
    std::string negatedTranslation(const std::string& delim) const 
    {
        return "jumpif " + std::string(term1) +
                relEnumStrs[Relations::negateRelop(op)] + std::string(term2) + " " + getData() + ";" + delim;
    }
    std::string condition(const std::string& delim) const 
    {
        return std::string(term1) + relEnumStrs[op] + std::string(term2) + delim;
    }
    std::string negatedCondition(const std::string& delim) const 
    {
        return std::string(term1) + relEnumStrs[Relations::negateRelop(op)] + std::string(term2) + delim;
    }
    void negate() {op = Relations::negateRelop(op);}
};

class InputVarCommand: public AbstractCommand
{
public:
    const std::unique_ptr<VarSetter> vs;

    InputVarCommand(std::unique_ptr<VarSetter> into, int linenum);
    std::string translation(const std::string& delim) const override{return "input " + getData() + ";" + delim;};

    std::unique_ptr<AbstractCommand> clone() override;

    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat) override;
};

class FunctionSymbol;
class PushCommand: public AbstractCommand
{
public:
    StringType stringType;
    FunctionSymbol* calledFunction;
    unsigned int pushedVars;

    PushCommand(const std::string& in, int linenum, FunctionSymbol* cf = nullptr, unsigned int numPushedLocalVars = 0):
            AbstractCommand(linenum), calledFunction(cf), pushedVars(numPushedLocalVars), stringType(getStringType(in))
    {
        setData(in);
        setType(CommandType::PUSH);
    }

    inline bool pushesState() const
    {
        return calledFunction != nullptr;
    }

    std::string translation(const std::string& delim) const override
    {
        if (pushesState()) return "push state " + getData() + ";" + delim;
        else return "push " + getData() + ";" + delim;
    }

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<PushCommand>(getData(), getLineNum(), calledFunction);
    }

    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat) override;
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
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat) override;
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
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat) override;
};

class EvaluateExprCommand: public AbstractCommand
{
public:
    std::unique_ptr<VarSetter> lhs;

    class Term
    {
    public:
        bool isLit;

        union
        {
            std::unique_ptr<VarGetter> vg;
            double d;
        };

        Term(std::string& toParse);
        Term(const Term& other);
        Term(const Term&& other);
        ~Term();

        std::string str() const;
    };

    Term term1;
    Term term2;

    ArithOp op;

    EvaluateExprCommand(std::unique_ptr<VarSetter> lh, Term t1, ArithOp o, Term t2, int linenum);
    EvaluateExprCommand(const EvaluateExprCommand& o);
    ~EvaluateExprCommand();
    std::unique_ptr<AbstractCommand> clone() override;
    std::string translation(const std::string& delim) const override;
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat) override;
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
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat) override;
};

class DeclareArrayCommand: public AbstractCommand
{
public:
    const unsigned long size;

    DeclareArrayCommand(const std::string& name, const unsigned long& n, int linenum): AbstractCommand(linenum), size(n)
    {
        if (size == 0) throw "arrays have size >0";
        setData(name);
        setType(CommandType::DECLAREVAR);
    }

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<DeclareArrayCommand>(getData(), size, getLineNum());
    }

    std::string translation(const std::string& delim) const override
    {
        return "double[" + std::to_string(size) + "] " + getData() + ";" + delim;
    }

    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat) override;
};

#endif