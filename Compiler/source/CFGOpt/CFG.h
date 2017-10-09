#ifndef PROJECT_CFG_H
#define PROJECT_CFG_H

#include <unordered_map>

#include "../compile/SymbolTable.h"
#include "../compile/Token.h"
#include "../Command.h"
#include "../compile/Reporter.h"

class FunctionSymbol;
class FunctionTable;

class CFGNode;

/*This class owns the nodes*/
class ControlFlowGraph
{
private:
    std::unordered_map<std::string, CFGNode*> currentNodes;
    CFGNode* first;
    CFGNode* last;
    Reporter& reporter;
    FunctionTable& functionTable;


public:
    ControlFlowGraph(Reporter& r, FunctionTable& fc) : reporter(r), functionTable(fc) {};
    ~ControlFlowGraph();
    CFGNode* createNode(const std::string& name, bool overwrite, bool last, FunctionSymbol* parentFunc);
    CFGNode* createNode(const std::string& name, bool overwrite, bool last);
    CFGNode* getNode(const std::string& name);
    void removeNode(std::string name);
    std::unordered_map<std::string, CFGNode*>& getCurrentNodes();
    void printSource();
    void printDotGraph();
    CFGNode* getFirst() const;
    CFGNode* getLast() const;
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
    std::vector<CFGNode*> pushingStates; //used to remove pushes if the node is being removed
    bool isLast;
    int jumpline;

public:
    CFGNode(ControlFlowGraph& p, FunctionSymbol* pf, std::string n, bool last = false);
    bool constProp(); //returns true if it bypassed some return
    bool addParent(CFGNode*); //returns false if parent was already in
    void removeParent(CFGNode*);
    void removeParent(const std::string&);

    void setInstructions(std::vector<std::unique_ptr<AbstractCommand>>& in);
    void setCompSuccess(CFGNode* compSuccess);
    void setCompFail(CFGNode* compFail);
    void setComp(std::unique_ptr<JumpOnComparisonCommand> comp);
    void setParentFunction(FunctionSymbol* pf);
    void setLast(bool last = true);

    void addPushingState(CFGNode* cfgn);
    //assumes the pop is handled elsewhere
    void removePushes();
    void removePushingState(const std::string& name);
    void replacePushes(const std::string& rep);
    void prepareToDie();

    //tries to merge with other (if it can fix the jumps so that it can do so)
    //returns true if successful
    bool swallowNode(CFGNode* other);

    JumpOnComparisonCommand* getComp();
    std::unordered_map<std::string, CFGNode*>& getPredecessors();
    CFGNode* getCompSuccess();
    CFGNode* getCompFail();
    int getJumpline() const;
    const std::string &getName() const;
    std::vector<std::unique_ptr<AbstractCommand>>& getInstrs();
    ControlFlowGraph& getParentGraph() const;
    FunctionSymbol* getParentFunction() const;
    void printSource(bool makeState = true, std::string delim = "\n");
    void printDotNode();
    bool isLastNode() const;
    bool isFirstNode() const;
};

#endif
