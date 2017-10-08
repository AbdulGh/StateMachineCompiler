#ifndef PROJECT_FUNCTIONSYMBOL_H
#define PROJECT_FUNCTIONSYMBOL_H

#include <vector>
#include <sstream>
#include <set>

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
    std::vector<std::string> vars; //used to save vars during function calls (todo make set)
    std::vector<std::shared_ptr<AbstractCommand>> currentInstrs;
    ControlFlowGraph& cfg;
    std::set<CFGNode*> returnTo;

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
    void giveNodesTo(FunctionSymbol* to);
    const std::shared_ptr<CFGNode>& getCurrentNode() const;
    const std::vector<std::string>& getVars();
    void addVar(std::string);

    //return stuff
    void addReturnSuccessor(CFGNode* other);
    void addReturnSuccessors(const std::set<CFGNode*>& newRet);
    void clearReturnSuccessors();
    void removeReturnSuccessor(const std::string& ret);
    void setReturnSuccessors(std::set<CFGNode*>& newRet);
    const std::set<CFGNode*>& getReturnSuccessors();

    //codegen
    void genNewState(std::string);
    void genEndState();
    void genPrint(std::string, int);
    void genJump(std::string, int);
    void genConditionalJump(std::string, std::string, Relations::Relop r, std::string, int);
    void genPush(std::string, int, FunctionSymbol* = nullptr);
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
    std::string removeUnderscoreWrappers(std::string underscored);
    Compiler& parent;
public:
    FunctionTable(Compiler& p) : parent(p) {}
    bool containsFunction(const std::string& funcName);
    FunctionSymbol* getFunction(const std::string& funcName);
    FunctionSymbol* getParentFunc(std::string stateName);
    FunctionSymbol* addFunction(VariableType returnType, std::vector<VariableType>& types, std::string& ident);
    void removeFunction(const std::string& bye);
    unsigned long getSize() {return functionTable.size();}
};


#endif
