#ifndef PROJECT_CFG_H
#define PROJECT_CFG_H

#include <unordered_map>
#include <set>

#include "../compile/SymbolTable.h"
#include "../compile/Token.h"
#include "../Command.h"
#include "../compile/Reporter.h"
#include "../compile/Functions.h"

class CFGNode;

//This class owns the nodes
typedef std::unordered_map<std::string, std::unique_ptr<CFGNode>>::iterator NodeMapIterator;
class ControlFlowGraph
{
private:
    std::unordered_map<std::string, std::unique_ptr<CFGNode>> currentNodes;
    CFGNode* first;
    CFGNode* last;
    Reporter& reporter;
    FunctionTable& functionTable;
    SymbolTable& symbolTable;

public:
    ControlFlowGraph(Reporter& r, FunctionTable& fc, SymbolTable& st) : reporter(r), functionTable(fc), symbolTable(st) {};
    const FunctionSymbol& getMain() const;
    CFGNode* createNode(const std::string& name, bool overwrite, bool last, FunctionSymbol* parentFunc);
    CFGNode* createNode(const std::string& name, bool overwrite, bool last);
    CFGNode* getNode(const std::string& name);
    NodeMapIterator removeNode(std::string name);
    NodeMapIterator removeNode(NodeMapIterator it);
    std::unordered_map<std::string, std::unique_ptr<CFGNode>>& getCurrentNodes();
    std::string getStructuredSource();
    std::string getDotGraph();
    std::string destroyStructureAndGetFinalSource();
    CFGNode* getFirst() const;
    CFGNode* getLast() const;
    Reporter& getReporter() const;
    void setFirst(const std::string& firstname);
    void setLast(const std::string& lastName);

    friend class CFGNode;
};

class CFGNode: public std::enable_shared_from_this<CFGNode>
{
private:
    std::string name;
    std::unique_ptr<JumpOnComparisonCommand> comp;
    std::unordered_map<std::string, CFGNode*> predecessors;
    CFGNode* compSuccess;
    CFGNode* compFail; //unconditional jump at the end of the node
    std::vector<std::unique_ptr<AbstractCommand>> instrs; //pointers to allow downcasting (avoid object slicing)
    ControlFlowGraph& parentGraph;
    FunctionSymbol* parentFunction;
    FunctionSymbol* calledFunctionSymbol = nullptr;
    std::set<std::pair<CFGNode*, FunctionSymbol*>> pushingStates; //used to remove pushes if the node is being removed
    bool isLast;
    int jumpline;

public:
    CFGNode(ControlFlowGraph& p, FunctionSymbol* pf, std::string n, bool last = false);
    bool constProp(std::unordered_map<std::string, Atom> assignments = std::unordered_map<std::string, Atom>()); //returns true if it bypassed some return
    bool addParent(CFGNode*); //returns false if parent was already in
    void removeParent(CFGNode*);
    void removeParent(const std::string&);
    void clearPredecessors(); //does not remove successor relationships

    void setInstructions(std::vector<std::unique_ptr<AbstractCommand>>& in);
    void setFunctionCall(FunctionSymbol* fc);
    void appendDeclatation(VariableType type, std::string varName);
    void setCompSuccess(CFGNode* compSuccess);
    void setCompFail(CFGNode* compFail);
    void setComp(std::unique_ptr<JumpOnComparisonCommand> comp);
    void setParentFunction(FunctionSymbol* pf);
    void setLast(bool last = true);

    void addFunctionCall(CFGNode* cfgn, FunctionSymbol* fs);
    void removePushes();
    void removeFunctionCall(const std::string& name, FunctionSymbol* fs);
    void replacePushes(const std::string& rep);
    unsigned int getNumPushingStates();
    bool noPreds(); //accounts for self loops
    FunctionSymbol* calledFunction();
    void prepareToDie();

    //tries to merge with other (if it can fix the jumps so that it can do so)
    //returns true if successful and relationship can be forgotten
    bool swallowNode(CFGNode* other);

    JumpOnComparisonCommand* getComp() const;
    const std::unordered_map<std::string, CFGNode*>& getPredecessorMap();
    std::vector<CFGNode*> getPredecessorVector() const;
    std::vector<CFGNode*> getSuccessorVector() const;
    CFGNode* getCompSuccess() const;
    CFGNode* getCompFail() const;
    int getJumpline() const;
    const std::string& getName() const;
    std::vector<std::unique_ptr<AbstractCommand>>& getInstrs();
    ControlFlowGraph& getParentGraph() const;
    FunctionSymbol* getParentFunction() const;
    std::string getSource(bool makeState = true, std::string delim = "\n", bool escape = false) const;
    std::string getDotNode();
    std::string getDotEdges();
    bool isLastNode() const;
    bool isFirstNode() const;
};

#endif
