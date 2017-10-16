#ifndef PROJECT_CFG_H
#define PROJECT_CFG_H

#include <unordered_map>
#include <set>

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
    std::unordered_map<std::string, std::unique_ptr<CFGNode>> currentNodes;
    CFGNode* first;
    CFGNode* last;
    Reporter& reporter;
    FunctionTable& functionTable;
    SymbolTable& symbolTable;


public:
    ControlFlowGraph(Reporter& r, FunctionTable& fc, SymbolTable& st) : reporter(r), functionTable(fc), symbolTable(st) {};
    CFGNode* createNode(const std::string& name, bool overwrite, bool last, FunctionSymbol* parentFunc);
    CFGNode* createNode(const std::string& name, bool overwrite, bool last);
    CFGNode* getNode(const std::string& name);
    void removeNode(std::string name);
    std::unordered_map<std::string, std::unique_ptr<CFGNode>>& getCurrentNodes();
    std::string getStructuredSource();
    std::string getDotGraph();
    std::string destroyStructureAndGetFinalSource();
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
    std::set<CFGNode*> pushingStates; //used to remove pushes if the node is being removed
    bool isLast;
    int jumpline;

public:
    CFGNode(ControlFlowGraph& p, FunctionSymbol* pf, std::string n, bool last = false);
    bool constProp(std::unordered_map<std::string, std::string> assignments = std::unordered_map<std::string, std::string>()); //returns true if it bypassed some return
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
    unsigned int getNumPushingStates();
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
    std::string getSource(bool makeState = true, std::string delim = "\n", bool escape = false);
    std::string getDotNode();
    bool isLastNode() const;
    bool isFirstNode() const;
};

#endif
