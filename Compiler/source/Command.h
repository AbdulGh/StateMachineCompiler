#ifndef COMMAND_H
#define COMMAND_H

#include <memory>

#include "compile/Token.h"

class VarWrapper;
class VarGetter;
class VarSetter;
namespace SymbolicExecution {class SymbolicExecutionFringe;}; //symbolic/SymbolicExecution.cpp

enum class CommandType{JUMP, CONDJUMP, RETURN, DECLAREVAR, PUSH, POP, ASSIGNVAR, EXPR, PRINT, INPUTVAR};
enum class StringType{ID, STRINGLIT, DOUBLELIT};

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
    StringType type;
    operator std::string() const;
    ~Atom();
};

//todo move type into AC constructor
class AbstractCommand
{
private:
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

    virtual const std::string& getState() const {throw "no state";}
    virtual void setState(const std::string& data) {throw "no state";}
    virtual const Atom& getAtom() const {throw "no atom";}
    virtual void setAtom(const Atom& data) {throw "no atom";}
    virtual const std::unique_ptr<VarSetter>& getVarSetter() const {throw "doesn't set var";}
    virtual void setVarSetter(std::unique_ptr<VarSetter> sv) {throw "doesn't set var";}

    CommandType getType() const
    {
        return commandType;
    }

    int getLineNum() const
    {
        return linenumber;
    }

    static StringType getStringType(const std::string& str)
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

class StateHoldingCommand : public AbstractCommand
{
protected:
    std::string state;
public:
    const std::string& getState() const override {return state;}
    void setState(const std::string& data) override {state = data;}
};

class AtomHoldingCommand : public AbstractCommand
{
protected:
    Atom atom;
public:
    const Atom& getAtom() const override {return atom;}
    void setAtom(const Atom& data) override {atom = data;}
};

class VarSettingCommand : public AbstractCommand
{
protected:
    std::unique_ptr<VarSetter> vs;
public:
    VarSettingCommand::VarSettingCommand(): vs{} {}

    const std::unique_ptr<VarSetter>& getVarSetter() const override {return move(vs);}
    void setVarSetter(std::unique_ptr<VarSetter> nvs) override {vs = move(nvs);}
};

class PrintCommand: public AbstractCommand, public AtomHoldingCommand
{
public:
    PrintCommand(Atom atom, int linenum) : AbstractCommand(linenum), AtomHoldingCommand::atom(std::move(atom))
    {
        setType(CommandType::PRINT);
    }

    std::string translation(const std::string& delim) const override {return "print " + std::string(atom) + ";" + delim;}

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<PrintCommand>(atom, getLineNum());
    }
};

class ReturnCommand: public AbstractCommand
{
public:
    ReturnCommand(int linenum) : AbstractCommand(linenum)
    {
        setType(CommandType::RETURN);
    }
    std::string translation(const std::string& delim) const override {return "return;" + delim;} //meta

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<ReturnCommand>(getLineNum());
    }
};

class JumpCommand: public AbstractCommand, public StateHoldingCommand
{
public:
    JumpCommand(const std::string& to, int linenum) : AbstractCommand(linenum)
    {
        setState(to);
        setType(CommandType::JUMP);
    }
    std::string translation(const std::string& delim) const override{return "jump " + state + ";" + delim;};

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<JumpCommand>(state, getLineNum());
    }
};

class JumpOnComparisonCommand: public AbstractCommand, public StateHoldingCommand
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
        if (term1.type != StringType::ID
            && term2.type == StringType::ID)
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
        return "jumpif " + std::string(term1) + relEnumStrs[op] + std::string(term2) + " " + state + ";" + delim;
    }
    std::string negatedTranslation(const std::string& delim) const 
    {
        return "jumpif " + std::string(term1) +
                relEnumStrs[Relations::negateRelop(op)] + std::string(term2) + " " + state + ";" + delim;
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

class InputVarCommand: public AbstractCommand, public VarSettingCommand
{
public:
    InputVarCommand(std::unique_ptr<VarSetter> into, int linenum);
    std::string translation(const std::string& delim) const override;
    std::unique_ptr<AbstractCommand> clone() override;
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat) override;
};

class FunctionSymbol;
class PushCommand: public AbstractCommand, public AtomHoldingCommand //todo split into push state/var
{
public:
    StringType stringType;
    FunctionSymbol* calledFunction;
    unsigned int pushedVars;

    PushCommand(const std::string& in, int linenum, FunctionSymbol* cf = nullptr, unsigned int numPushedLocalVars = 0):
    AbstractCommand(linenum), calledFunction(cf), pushedVars(numPushedLocalVars), stringType(getStringType(in)), atom(in)
    {
        setType(CommandType::PUSH);
    }

    inline bool pushesState() const
    {
        return calledFunction != nullptr;
    }

    std::string translation(const std::string& delim) const override
    {
        if (pushesState()) return "push state " + std::string(atom) + ";" + delim;
        else return "push " + std::string(atom) + ";" + delim;
    }

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<PushCommand>(atom, getLineNum(), calledFunction);
    }

    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat) override;
};

class PopCommand: public AbstractCommand, public VarSettingCommand
{
public:
    PopCommand(std::unique_ptr<VarSetter> into, int linenum);
    std::unique_ptr<AbstractCommand> clone() override;
    bool isEmpty() const {return vs != nullptr;}
    std::string translation(const std::string& delim) const override;
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat) override;
};


class AssignVarCommand: public AbstractCommand
{
public:
    std::unique_ptr<VarSetter> lhs;
    Atom rhs;

    AssignVarCommand(std::unique_ptr<VarSetter> lh, std::unique_ptr<VarGetter> rh, int linenum);
    AssignVarCommand(std::unique_ptr<VarSetter> lh, const std::string& rh, int linenum);
    std::unique_ptr<AbstractCommand> clone() override;
    std::string translation(const std::string& delim) const override;
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

        Term(const std::string& toParse);
        Term(const Term& other);
        Term(Term&& other);
        ~Term();

        void parse(const std::string& toParse);

        bool operator==(Term& o);

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


class DeclareCommand: public AbstractCommand
{
public:
    enum class DeclareType {VAR, ARRAY, POINTER};
    const DeclareType dt;
    const VariableType vt;
    DeclareCommand(DeclareType declareType, VariableType variableType, int linenum)
            :AbstractCommand(linenum), dt(declareType), vt(variableType)
    {
        setType(CommandType::DECLAREVAR);
    }
    virtual const std::string& getBaseName() const = 0;
};

class DeclareVarCommand: public AbstractCommand, public DeclareCommand
{
private:
    std::string name;
public:
    DeclareVarCommand(VariableType t, std::string n, int linenum)
            :DeclareCommand(DeclareType::VAR, t, linenum), name(move(n)) {}

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<DeclareVarCommand>(vt, name, getLineNum());
    }

    std::string translation(const std::string& delim) const override
    {
        return VariableTypeEnumNames[vt] + " " + name + ";" + delim;
    }

    const std::string& getBaseName() const override {return name;}

    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat) override;
};

class DeclareArrayCommand: public AbstractCommand, public DeclareCommand
{
private:
    std::string name;
public:
    const unsigned long size;

    DeclareArrayCommand(std::string n, const unsigned long& s, int linenum):
            DeclareCommand(DeclareType::ARRAY, DOUBLE, linenum), name(move(n)), size(s)
    {
        if (size == 0) throw "arrays have size >0";
    }

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<DeclareArrayCommand>(name, size, getLineNum());
    }

    std::string translation(const std::string& delim) const override
    {
        return "double[" + std::to_string(size) + "] " + name + ";" + delim;
    }

    const std::string& getBaseName() const override {return name;}

    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat) override;
};

#endif