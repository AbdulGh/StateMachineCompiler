#ifndef COMMAND_H
#define COMMAND_H

#include <memory>

#include "compile/Token.h"

class VarWrapper;
namespace SymbolicExecution {class SymbolicExecutionFringe;}; //symbolic/SymbolicExecution.cpp

enum class CommandType{JUMP, CONDJUMP, RETURN, DECLAREVAR, PUSH, POP, ASSIGNVAR, EXPR, PRINT, INPUTVAR, NONDET};

enum class StringType{ID, DOUBLELIT};
StringType getStringType(const std::string& str);

class Atom
{
private:
    double d;
    std::unique_ptr<VarWrapper> vptr;
    bool holding;
    StringType type;

public:
    Atom();
    ~Atom();
    explicit Atom(double d);
    explicit Atom(std::unique_ptr<VarWrapper>);
    Atom(const Atom& o);
    Atom(Atom&& o);
    Atom& operator=(const Atom& o);
    Atom& operator=(Atom&& o);
    bool isHolding() const;
    StringType getType() const; //todo replace this w/ isHolding
    double getLiteral() const;
    VarWrapper* getVarWrapper() const;
    void swap(Atom& a);
    void become(const Atom& a);
    void set(double sptr);
    void set(std::unique_ptr<VarWrapper> sptr);
    operator std::string() const;
    //used to put assignments in maps in dataflow
    bool operator<(const Atom& right) const;
    bool operator==(const Atom& right) const;
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
    virtual bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat = false);

    virtual const std::string& getString() const {throw std::runtime_error("no state");}
    virtual void setString(const std::string& data) {throw std::runtime_error("no state");}
    virtual Atom& getAtom() {throw std::runtime_error("no atom");}
    virtual void setAtom(Atom data) {throw std::runtime_error("no atom");}
    virtual const std::unique_ptr<VarWrapper>& getVarWrapper() const {throw std::runtime_error("doesn't set var");}
    virtual void setVarWrapper(std::unique_ptr<VarWrapper> sd) {throw std::runtime_error("doesn't set var");}

    CommandType getType() const
    {
        return commandType;
    }

    int getLineNum() const
    {
        return linenumber;
    }
};

class StringHoldingCommand : public AbstractCommand
{
protected:
    std::string state;
public:
    StringHoldingCommand(std::string s, int linenum): AbstractCommand(linenum), state(move(s)) {}
    virtual ~StringHoldingCommand();
    const std::string& getString() const override {return state;}
    void setString(const std::string& data) override {state = data;}
};

class AtomHoldingCommand : public AbstractCommand
{
protected:
    Atom atom;
public:
    AtomHoldingCommand(Atom a, int linenum): AbstractCommand(linenum), atom{std::move(a)} {}
    virtual ~AtomHoldingCommand();
    Atom& getAtom() override {return atom;}
    void setAtom(Atom data) override {atom = Atom(data);}
};

class WrapperHoldingCommand : public AbstractCommand
{
protected:
    std::unique_ptr<VarWrapper> vs;
public:
    WrapperHoldingCommand(std::unique_ptr<VarWrapper> VarWrapper, int linenum);
    virtual ~WrapperHoldingCommand();

    const std::unique_ptr<VarWrapper>& getVarWrapper() const override {return vs;}
    void setVarWrapper(std::unique_ptr<VarWrapper> nvs) override;
};

class PrintAtomCommand: public AtomHoldingCommand
{
public:
    PrintAtomCommand(Atom atom, int linenum) : AtomHoldingCommand(atom, linenum)
    {
        setType(CommandType::PRINT);
    }

    std::string translation(const std::string& delim) const override {return "print " + std::string(atom) + ";" + delim;}

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<PrintAtomCommand>(atom, getLineNum());
    }
};

class PrintLiteralCommand: public StringHoldingCommand
{
public:
    PrintLiteralCommand(std::string s, int linenum) : StringHoldingCommand(move(s), linenum)
    {
        setType(CommandType::PRINT);
    }

    std::string translation(const std::string& delim) const override {return "print " + getString() + ";" + delim;}

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<PrintLiteralCommand>(getString(), getLineNum());
    }
};

class ReturnCommand: public StringHoldingCommand
{
public:
    ReturnCommand(int linenum) : StringHoldingCommand("return", linenum)
    {
        setType(CommandType::RETURN);
    }
    std::string translation(const std::string& delim) const override {return "return;" + delim;} //meta

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<ReturnCommand>(getLineNum());
    }
};

class JumpCommand: public StringHoldingCommand
{
public:
    JumpCommand(const std::string& to, int linenum): StringHoldingCommand(to, linenum)
    {
        setType(CommandType::JUMP);
    }
    std::string translation(const std::string& delim) const override{return "jump " + state + ";" + delim;};

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<JumpCommand>(state, getLineNum());
    }
};

class JumpOnComparisonCommand: public StringHoldingCommand
{
public:
    Atom term1;
    Atom term2;

    Relations::Relop op;

    JumpOnComparisonCommand(const std::string& st, std::unique_ptr<VarWrapper> t1,
                            std::unique_ptr<VarWrapper> t2, Relations::Relop o, int linenum);

    JumpOnComparisonCommand(const std::string& st, std::unique_ptr<VarWrapper> t1,
                            double t2, Relations::Relop o, int linenum);

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
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat) override;
};

class FunctionSymbol;
class PushCommand: public AbstractCommand
{
private:

        Atom atom;
        std::string s;

public:
    FunctionSymbol* calledFunction;
    unsigned int pushedVars;

    PushCommand(std::string in, int linenum, FunctionSymbol* cf = nullptr, unsigned int numPushedLocalVars = 0):
            AbstractCommand(linenum), s(move(in)), calledFunction(cf), pushedVars(numPushedLocalVars)
    {
        setType(CommandType::PUSH);
        if (calledFunction == nullptr) throw std::runtime_error("use other constructor");
    }

    PushCommand(Atom in, int linenum);
    std::unique_ptr<AbstractCommand> clone() override;
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat) override;

    inline bool pushesState() const
    {
        return calledFunction != nullptr;
    }

    std::string translation(const std::string& delim) const override;

    const std::string& getString() const override
    {
        if (!pushesState()) throw std::runtime_error("doesn't push state");
        return s;
    }

    void setAtom(Atom data) override //this could be done better
    {
        if (pushesState()) throw std::runtime_error("no atom");
        atom = std::move(data);
    }

    Atom& getAtom() override
    {
        if (pushesState()) throw std::runtime_error("no atom");
        return atom;
    }

};

class PopCommand: public WrapperHoldingCommand
{
public:
    PopCommand(std::unique_ptr<VarWrapper> into, int linenum);
    void clear();
    std::unique_ptr<AbstractCommand> clone() override;
    bool isEmpty() const {return vs == nullptr;}
    std::string translation(const std::string& delim) const override;
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat) override;
};

class AssignVarCommand: public AbstractCommand
{
private:
    Atom atom;
    std::unique_ptr<VarWrapper> vs;

public:
    AssignVarCommand(std::unique_ptr<VarWrapper> lh, Atom at, int linenum);
    AssignVarCommand(std::unique_ptr<VarWrapper> lh, std::unique_ptr<VarWrapper> rh, int linenum);
    Atom& getAtom() override {return atom;}
    void setAtom(Atom data) override {atom = std::move(data);}
    const std::unique_ptr<VarWrapper>& getVarWrapper() const override {return vs;}
    void setVarWrapper(std::unique_ptr<VarWrapper> sd) override;
    std::unique_ptr<AbstractCommand> clone() override;
    std::string translation(const std::string& delim) const override;
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat) override;
};

class EvaluateExprCommand: public WrapperHoldingCommand
{
public:
    Atom term1;
    Atom term2;

    ArithOp op;

    EvaluateExprCommand(std::unique_ptr<VarWrapper> lh, Atom t1, ArithOp o, Atom t2, int linenum);
    EvaluateExprCommand(const EvaluateExprCommand& o);
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
    enum class DeclareType {VAR, ARRAY};
    const DeclareType dt;
    DeclareCommand(DeclareType declareType, int linenum)
            :AbstractCommand(linenum), dt(declareType)
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
    DeclareVarCommand(std::string n, int linenum)
            :DeclareCommand(DeclareType::VAR, linenum), name(move(n))
    {}

    std::unique_ptr<AbstractCommand> clone() override
    {
        return std::make_unique<DeclareVarCommand>( name, AbstractCommand::getLineNum());
    }

    std::string translation(const std::string& delim) const override
    {
        return "double " + name + ";" + delim;
    }

    const std::string& getBaseName() const override {return name;}

    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat) override;
};

class DeclareArrayCommand: public DeclareCommand
{
private:
    std::string name;
public:
    const unsigned long size;

    DeclareArrayCommand(std::string n, const unsigned long& s, int linenum):
            DeclareCommand(DeclareType::ARRAY, linenum), name(move(n)), size(s)
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

class NondetCommand : public AbstractCommand
{
    union
    {
        std::string s;
        std::unique_ptr<VarWrapper> varWrapper;
    };
public:
    const bool holding;

    NondetCommand(std::unique_ptr<VarWrapper> vw, int linenum);

    NondetCommand(std::string stringie, int linenum):
            AbstractCommand(linenum), s(move(stringie)), holding(false)
    {setType(CommandType::NONDET);}

    ~NondetCommand();

    std::unique_ptr<AbstractCommand> clone() override;
    std::string translation(const std::string& delim) const override;
    bool acceptSymbolicExecution(std::shared_ptr<SymbolicExecution::SymbolicExecutionFringe> sef, bool repeat) override;

    const std::unique_ptr<VarWrapper>& getVarWrapper() const override
    {
        if (!holding) throw std::runtime_error("i fall ovre");
        return varWrapper;
    }

    const std::string& getString() const override
    {
        if (holding) throw std::runtime_error("nondet-ing var");
        return s;
    }

    void setVarWrapper(std::unique_ptr<VarWrapper> sd) override;
};


#endif