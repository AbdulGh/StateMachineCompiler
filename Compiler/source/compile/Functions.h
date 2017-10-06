#ifndef PROJECT_FUNCTIONSYMBOL_H
#define PROJECT_FUNCTIONSYMBOL_H

#include <vector>
#include <sstream>

#include "Token.h"
#include "../CFGOpt/CFG.h"
#include "../Command.h"

class Compiler;

class FunctionSymbol : public std::enable_shared_from_this<FunctionSymbol>
{
private:
    VariableType returnType;
    std::vector<VariableType> paramTypes;
    int currentStates;
    std::string prefix;
    bool endedState;
    std::shared_ptr<CFGNode> lastNode;
    std::shared_ptr<CFGNode> firstNode;
    std::shared_ptr<CFGNode> currentNode;
    std::vector<std::string> vars; //used to save vars during function calls
    std::vector<std::shared_ptr<AbstractCommand>> currentInstrs;
    ControlFlowGraph& cfg;
    std::vector<CFGNode*> returnTo;

public:
    FunctionSymbol(VariableType returnType, std::vector<VariableType> types, std::string ident, ControlFlowGraph& cfg);
    const std::string newStateName();
    const std::string& getPrefix() const;
    bool checkTypes(std::vector<VariableType>& potential);
    bool isOfType(VariableType c);
    VariableType getReturnType() const;
    const std::shared_ptr<CFGNode>& getLastNode();
    void setLastNode(const std::shared_ptr<CFGNode>& lastNode);
    const std::shared_ptr<CFGNode>& getFirstNode();
    void setFirstNode(const std::shared_ptr<CFGNode>& firstNode);
    void giveNodesTo(FunctionSymbol* to); //todo use this
    const std::shared_ptr<CFGNode>& getCurrentNode() const;
    const std::vector<std::string>& getVars();
    void addVar(std::string);

    //return stuff
    std::vector<CFGNode*>& getReturnTo();
    void addReturnSuccessor(CFGNode* other);
    void addReturnSuccessors(const std::vector<CFGNode*>& newRet);
    void clearReturnSuccessors();
    void removeReturnSuccessor(const std::string& ret);
    void setReturnSuccessors(std::vector<CFGNode*>& newRet);
    const std::vector<CFGNode*>& getReturnSuccessors();

    //codegen
    void genNewState(std::string);
    void genEndState();
    void genPrint(std::string, int);
    void genJump(std::string, int);
    void genConditionalJump(std::string, std::string, Relations::Relop r, std::string, int);
    void genPush(PushCommand::PushType, std::string, int);
    void genPop(std::string, int);
    void genReturn(int);
    void genInput(std::string, int);
    void genExpr(std::string lh, std::string t1, Op o, std::string t2, int);
    void genVariableDecl(VariableType t, std::string n, int);
    void genAssignment(std::string LHS, std::string RHS, int);
    void addCommand(std::shared_ptr<AbstractCommand> ac);
    void addCommands(std::vector<std::shared_ptr<AbstractCommand>> acs);
};

class FunctionTable
{
private:
    std::unordered_map<std::string, std::unique_ptr<FunctionSymbol>> functionTable;
    Compiler& parent;
public:
    FunctionTable(Compiler& p) : parent(p) {}
    bool containsFunction(const std::string& funcName);
    FunctionSymbol* getFunction(const std::string& funcName);
    FunctionSymbol* getParentFunc(std::string stateName);
    FunctionSymbol* addFunction(VariableType returnType, std::vector<VariableType>& types, std::string& ident);
    unsigned long getSize() {return functionTable.size();}
};


#endif
