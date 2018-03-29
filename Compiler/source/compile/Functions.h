#ifndef PROJECT_FUNCTIONSYMBOL_H
#define PROJECT_FUNCTIONSYMBOL_H

#include <vector>
#include <sstream>
#include <set>
#include <unordered_map>
#include <memory>

#include "Token.h"
#include "../Command.h"

class Compiler;
class CFGNode;
class ControlFlowGraph;
class VarWrapper;

struct FunctionCall;
class FunctionSymbol : public std::enable_shared_from_this<FunctionSymbol>
{
private:
    class FunctionVars //used to save vars during function calls
    {
    private:
        std::unique_ptr<FunctionVars> parent;
        std::set<VarWrapper*> vars;

    public:
        explicit FunctionVars(std::unique_ptr<FunctionVars> p = nullptr);
        ~FunctionVars();
        const std::set<VarWrapper*> getVarSet();
        std::unique_ptr<FunctionVars> moveScope();
        void addVar(VarWrapper* varN);
    };

    VariableType returnType;
    std::vector<VariableType> paramTypes;
    int currentStateNum;
    std::string prefix;
    std::string ident;
    bool endedState;
    CFGNode* lastNode;
    CFGNode* firstNode;
    CFGNode* currentNode;
    std::vector<std::unique_ptr<AbstractCommand>> currentInstrs;
    std::unique_ptr<FunctionVars> currentVarScope;
    ControlFlowGraph& cfg;
    std::set<std::unique_ptr<FunctionCall>> calls;

public:
    FunctionSymbol(VariableType returnType, std::vector<VariableType> types, std::string ident, std::string prefix, ControlFlowGraph& cfg);
    const std::string newStateName();
    const std::string& getPrefix() const;
    const std::string& getIdent() const;
    bool checkTypes(std::vector<VariableType>& potential);
    bool isOfType(VariableType c);
    VariableType getReturnType() const;
    CFGNode* getLastNode();
    void setLastNode(CFGNode* lastNode); //deals w/ parent/pred stuff
    CFGNode* getFirstNode();
    void setFirstNode(CFGNode* firstNode);
    CFGNode* getCurrentNode() const;
    const std::set<VarWrapper*> getVars();
    void addVar(VarWrapper* id);
    unsigned int numParams();
    unsigned int numCalls();

    void pushScope();
    void popScope();

    //deals w/ pushes & pops
    bool mergeInto(FunctionSymbol *to);

    //return stuff
    FunctionCall* addFunctionCall(CFGNode* calling, CFGNode* returnTo, unsigned int numPushedVars);
    void replaceReturnState(CFGNode* going, CFGNode* replaceWith);
    const std::set<std::unique_ptr<FunctionCall>>& getFunctionCalls() const;
    void clearFunctionCalls();
    void removeFunctionCall(const std::string& calling, const std::string& ret, bool fixCalling = true);
    void forgetFunctionCall(const std::string& calling, const std::string& ret);
    std::vector<CFGNode*> getNodesReturnedTo();
    FunctionCall* getOnlyFunctionCall();

    //codegen
    void genNewState(std::string);
    void genEndState();
    void genPrint(Atom a, int linenum);
    void genJump(std::string, int linenum);
    void genConditionalJump(std::string state, std::unique_ptr<VarWrapper> lh,
                            Relations::Relop r, std::unique_ptr<VarWrapper> rh, int linenum);
    void genPush(std::string toPush, int, FunctionSymbol* calledFuntion = nullptr);
    void genPush(std::unique_ptr<VarWrapper> toPush, int linenum);
    void genPop(std::unique_ptr<VarWrapper> vs, int linenum);
    void genReturn(int linenum);
    void genInput(std::unique_ptr<VarWrapper>, int linenum);
    void genExpr(std::unique_ptr<VarWrapper> lh, Term& t1, ArithOp o, Term& t2, int linenum);
    void genVariableDecl(VariableType t, std::string n, int linenum);
    void genArrayDecl(std::string name, unsigned long int size, int linenum);
    void genAssignment(std::unique_ptr<VarWrapper> LHS, std::string RHS, int linenum);
    void genAssignment(std::unique_ptr<VarWrapper> LHS, std::unique_ptr<VarWrapper> RHS, int linenum);
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

struct FunctionCall
{
    CFGNode* caller;
    CFGNode* returnTo;
    FunctionSymbol* calledFunction;
    unsigned int numPushedVars;

    FunctionCall(CFGNode* callerNode, CFGNode* returnToNode, unsigned int numLocalVars, FunctionSymbol* cf):
            caller(callerNode), returnTo(returnToNode), numPushedVars(numLocalVars), calledFunction(cf) {}

    bool operator< (const FunctionCall& r) const;
};

#endif
