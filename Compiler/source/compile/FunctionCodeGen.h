#ifndef PROJECT_FUNCTIONSYMBOL_H
#define PROJECT_FUNCTIONSYMBOL_H

#include <vector>
#include <sstream>

#include "Token.h"
#include "../CFGOpt/CFG.h"
#include "../Command.h"

class FunctionCodeGen : public std::enable_shared_from_this<FunctionCodeGen>
{
private:
    VariableType returnType;
    std::vector<VariableType> paramTypes;
    int currentStates;
    std::string ident;
    bool endedState;
    std::shared_ptr<CFGNode> lastNode;
    std::shared_ptr<CFGNode> firstNode;
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
    const std::shared_ptr<CFGNode> &getLastNode();
    void setLastNode(const std::shared_ptr<CFGNode> &lastNode);
    const std::shared_ptr<CFGNode> &getFirstNode();
    void setFirstNode(const std::shared_ptr<CFGNode> &firstNode);

    //codegen
    void genNewState(std::string);
    std::shared_ptr<CFGNode> genEndState();
    void genPrint(std::string, int);
    void genJump(std::string, int);
    void genConditionalJump(std::string, std::string, Relations::Relop r, std::string, int);
    void genPush(std::string, int);
    void genPop(std::string, int);
    void genReturn(int);
    void genInput(std::string, int);
    void genExpr(std::string lh, std::string t1, Op o, std::string t2, int);
    void genVariableDecl(VariableType t, std::string n, int);
    void genAssignment(std::string LHS, std::string RHS, int);
    void addCommand(std::shared_ptr<AbstractCommand> ac);
};

typedef std::shared_ptr<FunctionCodeGen> FunctionPointer;

#endif
