#ifndef PROJECT_FUNCTIONSYMBOL_H
#define PROJECT_FUNCTIONSYMBOL_H

#include <vector>
#include <sstream>

#include "Token.h"
#include "../CFGOpt/CFG.h"
#include "../Command.h"

class FunctionCodeGen
{
private:
    VariableType returnType;
    std::vector<VariableType> paramTypes;
    int currentStates;
    std::string ident;
    bool endedState;

    //std::vector<State> finStates;
    std::vector<std::shared_ptr<AbstractCommand>> currentInstrs;
    std::string currentStateName;
    ControlFlowGraph& cfg;
public:
    FunctionCodeGen(VariableType returnType, std::vector<VariableType> types, std::string ident, ControlFlowGraph& cfg);
    const std::string newStateName();
    const std::string& getIdentifier() const;
    bool checkTypes(std::vector<VariableType>& potential);
    bool isOfType(VariableType c);
    VariableType getReturnType() const;
    //std::vector<State>& getFinStates();

    //codegen
    void genNewState(std::string);
    void genEndState();
    void genPrint(std::string);
    void genJump(std::string);
    void genConditionalJump(std::string, std::string, Relop r, std::string);
    void genPush(std::string);
    void genPop(std::string);
    void genReturn();
    void genInput(std::string);
    void genExpr(std::string lh, std::string t1, Op o, std::string t2);
    void genVariableDecl(VariableType t, std::string n);
    void genAssignment(std::string LHS, std::string RHS);
    void addCommand(std::shared_ptr<AbstractCommand> ac);
};

typedef std::shared_ptr<FunctionCodeGen> FunctionPointer;

#endif
