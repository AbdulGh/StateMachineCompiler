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
    std::string ident;
    bool endedState;
    CFGNode* lastNode;
    CFGNode* firstNode;
    CFGNode* currentNode;
    std::set<std::string> vars; //used to save vars during function calls
    std::vector<std::unique_ptr<AbstractCommand>> currentInstrs;
    ControlFlowGraph& cfg;
    std::set<CFGNode*> returnTo;

public:
    FunctionSymbol(VariableType returnType, std::vector<VariableType> types, std::string ident, std::string prefix, ControlFlowGraph& cfg);
    const std::string newStateName();
    const std::string& getPrefix() const;
    const std::string& getIdent() const;
    bool checkTypes(std::vector<VariableType>& potential);
    bool isOfType(VariableType c);
    VariableType getReturnType() const;
    CFGNode* getLastNode();
    void setLastNode(CFGNode* lastNode);
    CFGNode* getFirstNode();
    void setFirstNode(CFGNode* firstNode);
    void giveNodesTo(FunctionSymbol* to);
    CFGNode* getCurrentNode() const;
    const std::set<std::string>& getVars();
    void addVar(const std::string& id);

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
    void genPush(std::string, int, FunctionSymbol* calledFuntion = nullptr);
    void genPop(std::string, int);
    void genReturn(int);
    void genInput(std::string, int);
    void genExpr(std::string lh, std::string t1, Op o, std::string t2, int);
    void genVariableDecl(VariableType t, std::string n, int);
    void genAssignment(std::string LHS, std::string RHS, int);
    void addCommand(std::unique_ptr<AbstractCommand> ac);
    void addCommands(std::vector<std::unique_ptr<AbstractCommand>>& acs);
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
