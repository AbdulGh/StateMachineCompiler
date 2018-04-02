#ifndef COMMAND_H
#define COMMAND_H

#include <memory>

#include "compile/Token.h"

class VarWrapper;
namespace SymbolicExecution {class SymbolicExecutionFringe;}; //symbolic/SymbolicExecution.cpp

enum class CommandType{JUMP, CONDJUMP, RETURN, DECLAREVAR, PUSH, POP, ASSIGNVAR, EXPR, PRINT, INPUTVAR};

enum class StringType{ID, STRINGLIT, DOUBLELIT};
StringType getStringType(const std::string& str);

class Atom
{
private:
    union
    {
        std::string* sptr;
        VarWrapper* vptr;
    };

    bool holding;
    StringType type;


public:
    explicit Atom(const std::string&, bool allowEmpty = false);
    explicit Atom(std::unique_ptr<VarWrapper>);
    Atom(const Atom& o);
    Atom(Atom&& o);
    Atom& operator=(Atom& o) = delete;
    Atom& operator=(Atom&& o);
    bool isHolding() const;
    StringType getType() const;
    const std::string* getString() const;
    VarWrapper* getVarWrapper() const;
    void swap(Atom& a);
    void become(const Atom& a);
    void set(const std::string& sptr);
    void set(std::unique_ptr<VarWrapper> sptr);
    void resetRepeatBounds(SymbolicExecution::SymbolicExecutionFringe* sef);
    operator std::string() const;
    //used to put assignments in maps in dataflow
    bool operator<(const Atom& right) const;
    bool operator==(const Atom& right) const;
    ~Atom();
};

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

    virtual const std::string& getState() const {throw std::runtime_error("no state");}
    virtual void setState(const std::string& data) {throw std::runtime_error("no state");}
    virtual Atom& getAtom() {throw std::runtime_error("no atom");}
    virtual void setAtom(Atom data) {throw std::runtime_error("no atom");}
    virtual const std::unique_ptr<VarWrapper>& getVarWrapper() const {throw std::runtime_error("doesn't set var");}
    virtual void setVarWrapper(std::unique_ptr<VarWrapper> sv) {throw std::runtime_error("doesn't set var");}

    CommandType getType() const
    {
        return commandType;
    }

    int getLineNum() const
    {
        return linenumber;
    }
};

class StateHoldingCommand : public AbstractCommand
{
protected:
    std::string state;
public:
    StateHoldingCommand(std::string s, int linenum): AbstractCommand(linenum), state(s) {}
    const std::string& getState() const override {return state;}
    void setState(const std::string& data) override {state = data;}
};

class AtomHoldingCommand : public AbstractCommand
{
protected:
    Atom atom;
public:
    AtomHoldingCommand(Atom a, int linenum): AbstractCommand(linenum), atom(std::move(a)) {}
    Atom& getAtom() override {return atom;}
    void setAtom(Atom data) override {atom = std::move(data);}
};

class WrapperHoldingCommand : public AbstractCommand
{
protected:
    std::unique_ptr<VarWrapper> vs;
public:
    WrapperHoldingCommand(std::unique_ptr<VarWrapper> VarWrapper, int linenum);

    const std::unique_ptr<VarWrapper>& getVarWrapper() const override {return vs;}
    void setVarWrapper(std::unique_ptr<VarWrapper> nvs) override;
};

class PrintCommand: public AtomHoldingCommand
{
public:
    PrintCommand(Atom atom, int linenum) : AtomHoldingCommand(atom, linenum)
    {
        setType(CommandType::PRINT);
    }

    std::string translation(const std::string& delim) const override {return "print " + std::string(atom) + ";" + delim;}

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<PrintCommand>(atom, getLineNum());
    }
};

class ReturnCommand: public StateHoldingCommand
{
public:
    ReturnCommand(int linenum) : StateHoldingCommand("return", linenum)
    {
        setType(CommandType::RETURN);
    }
    std::string translation(const std::string& delim) const override {return "return;" + delim;} //meta

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<ReturnCommand>(getLineNum());
    }
};

class JumpCommand: public StateHoldingCommand
{
public:
    JumpCommand(const std::string& to, int linenum): StateHoldingCommand(to, linenum)
    {
        setType(CommandType::JUMP);
    }
    std::string translation(const std::string& delim) const override{return "jump " + state + ";" + delim;};

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<JumpCommand>(state, getLineNum());
    }
};

class JumpOnComparisonCommand: public StateHoldingCommand
{
public:
    Atom term1;
    Atom term2;

    Relations::Relop op;

    JumpOnComparisonCommand(const std::string& st, std::unique_ptr<VarWrapper> t1,
                            std::unique_ptr<VarWrapper> t2, Relations::Relop o, int linenum);

    JumpOnComparisonCommand(const std::string& st, std::unique_ptr<VarWrapper> t1,
                            const std::string& t2, Relations::Relop o, int linenum);

    JumpOnComparisonCommand(const JumpOnComparisonCommand& jocc);

    void makeGood()
    {
        if (term1.getType() != StringType::ID
            && term2.getType() == StringType::ID) term1.swap(term2);
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

class InputVarCommand: public WrapperHoldingCommand
{
public:
    InputVarCommand(std::unique_ptr<VarWrapper> into, int linenum);
    std::string translation(const std::string& delim) const override;
    std::unique_ptr<AbstractCommand> clone() override;
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat) override;
};

class FunctionSymbol;
class PushCommand: public AbstractCommand
{
private:
    std::unique_ptr<Atom> atom;
    std::string state;

public:
    StringType stringType;
    FunctionSymbol* calledFunction;
    unsigned int pushedVars;

    PushCommand(const std::string& in, int linenum, FunctionSymbol* cf = nullptr, unsigned int numPushedLocalVars = 0):
            AbstractCommand(linenum), calledFunction(cf), pushedVars(numPushedLocalVars), stringType(getStringType(in))
    {
        if (!calledFunction) atom = std::make_unique<Atom>(in);
        else state = in;
        setType(CommandType::PUSH);
    }

    PushCommand(std::unique_ptr<VarWrapper> in, int linenum);
    std::unique_ptr<AbstractCommand> clone() override;
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat) override;

    inline bool pushesState() const
    {
        return calledFunction != nullptr;
    }

    std::string translation(const std::string& delim) const override
    {
        if (pushesState()) return "push state " + state + ";" + delim;
        else return "push " + std::string(*atom) + ";" + delim;
    }

    const std::string& getState() const override
    {
        if (!pushesState()) throw std::runtime_error("doesn't push state");
        return state;
    }

    Atom& getAtom() override
    {
        if (pushesState()) throw std::runtime_error("doesn't push atom");
        return *atom;
    }

    void setAtom(Atom data) override {atom = std::make_unique<Atom>(data);}

};

class PopCommand: public WrapperHoldingCommand
{
public:
    PopCommand(std::unique_ptr<VarWrapper> into, int linenum);
    void clear();
    std::unique_ptr<AbstractCommand> clone() override;
    bool isEmpty() const {return vs == nullptr;}
    std::string translation(const std::string& delim) const override;
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat) override;
};

class AssignVarCommand: public AbstractCommand
{
private:
    Atom atom;
    std::unique_ptr<VarWrapper> vs;

public:
    AssignVarCommand(std::unique_ptr<VarWrapper> lh, Atom& at, int linenum);
    AssignVarCommand(std::unique_ptr<VarWrapper> lh, std::unique_ptr<VarWrapper> rh, int linenum);
    AssignVarCommand(std::unique_ptr<VarWrapper> lh, const std::string& rh, int linenum);
    Atom& getAtom() override {return atom;}
    void setAtom(Atom data) override {atom = std::move(data);}
    const std::unique_ptr<VarWrapper>& getVarWrapper() const override {return vs;}
    void setVarWrapper(std::unique_ptr<VarWrapper> sv) override;
    std::unique_ptr<AbstractCommand> clone() override;
    std::string translation(const std::string& delim) const override;
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat) override;
};

class Term
{
public:
    bool isLit;

    union
    {
        std::unique_ptr<VarWrapper> vg;
        double d;
    };

    Term(const std::string& toParse);
    Term(double doub);
    Term(const Term& other);
    Term(std::unique_ptr<VarWrapper> vg);
    Term(Term&& other);
    ~Term();
    Term& operator=(Term& o) = delete;
    Term& operator=(Term&& o) = delete;

    void parse(const std::string& toParse);

    bool operator==(Term& o);

    std::string str() const;
};

class
EvaluateExprCommand: public WrapperHoldingCommand
{
public:
    Term term1;
    Term term2;

    ArithOp op;

    EvaluateExprCommand(std::unique_ptr<VarWrapper> lh, Term t1, ArithOp o, Term t2, int linenum);
    EvaluateExprCommand(const EvaluateExprCommand& o);
    ~EvaluateExprCommand() override;
    EvaluateExprCommand(EvaluateExprCommand&& o) = delete;
    EvaluateExprCommand& operator=(EvaluateExprCommand& o) = delete;
    EvaluateExprCommand& operator=(EvaluateExprCommand&& o) = delete;
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

class DeclareVarCommand: public DeclareCommand
{
private:
    std::string name;
public:
    DeclareVarCommand(VariableType t, std::string n, int linenum)
            :DeclareCommand(DeclareType::VAR, t, linenum), name(move(n))
    {}

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<DeclareVarCommand>(vt, name, AbstractCommand::getLineNum());
    }

    std::string translation(const std::string& delim) const override
    {
        return VariableTypeEnumNames[vt] + " " + name + ";" + delim;
    }

    const std::string& getBaseName() const override {return name;}

    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> svs, bool repeat) override;
};

class DeclareArrayCommand: public DeclareCommand
{
private:
    std::string name;
public:
    const unsigned long size;

    DeclareArrayCommand(std::string n, const unsigned long& s, int linenum):
            DeclareCommand(DeclareType::ARRAY, DOUBLE, linenum), name(move(n)), size(s)
    {
        if (size == 0) throw std::runtime_error("arrays have size >0");
    }

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<DeclareArrayCommand>(name, size, AbstractCommand::getLineNum());
    }

    std::string translation(const std::string& delim) const override
    {
        return "double[" + std::to_string(size) + "] " + name + ";" + delim;
    }

    const std::string& getBaseName() const override {return name;}

    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat) override;
};

#endif