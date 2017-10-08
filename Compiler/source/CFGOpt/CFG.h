#ifndef PROJECT_CFG_H
#define PROJECT_CFG_H

#include <unordered_map>

#include "../compile/SymbolTable.h"
#include "../compile/Token.h"
#include "../Command.h"
#include "../compile/Reporter.h"

class FunctionSymbol;
class FunctionTable;
//todo use unique pointers here

class CFGNode;
class ControlFlowGraph
{
private:
    std::unordered_map<std::string, std::shared_ptr<CFGNode>> currentNodes;
    std::shared_ptr<CFGNode> first;
    std::shared_ptr<CFGNode> last;
    Reporter& reporter;
    FunctionTable& functionTable;


public:
    ControlFlowGraph(Reporter& r, FunctionTable& fc) : reporter(r), functionTable(fc) {};
    std::shared_ptr<CFGNode> createNode(const std::string& name, bool overwrite, bool last, FunctionSymbol* parentFunc);
    std::shared_ptr<CFGNode> createNode(const std::string& name, bool overwrite, bool last);
    std::shared_ptr<CFGNode> getNode(const std::string& name);
    void removeNode(std::string name);
    std::unordered_map<std::string, std::shared_ptr<CFGNode>>& getCurrentNodes();
    void printSource();
    void printDotGraph();
    std::shared_ptr<CFGNode> getFirst() const;
    std::shared_ptr<CFGNode> getLast() const;
    void setFirst(const std::string& firstname);
    void setLast(const std::string& lastName);

    friend class CFGNode;

};

class CFGNode: public std::enable_shared_from_this<CFGNode>
{
private:
    std::string name;
    std::shared_ptr<JumpOnComparisonCommand> comp;
    std::unordered_map<std::string, std::shared_ptr<CFGNode>> predecessors;
    std::shared_ptr<CFGNode> compSuccess;
    std::shared_ptr<CFGNode> compFail; //unconditional jump at the end of the node
    std::vector<std::shared_ptr<AbstractCommand>> instrs; //shared_ptrs to allow downcasting
    ControlFlowGraph& parentGraph;
    FunctionSymbol* parentFunction;
    std::vector<std::shared_ptr<CFGNode>> pushingStates; //used to remove pushes if the node is being removed
    bool isLast;
    int jumpline;

public:
    CFGNode(ControlFlowGraph& p, FunctionSymbol* pf, std::string n, bool last = false);
    bool constProp(); //returns true if it bypassed some return
    bool addParent(std::shared_ptr<CFGNode>); //returns false if parent was already in
    void removeParent(std::shared_ptr<CFGNode>);
    void removeParent(const std::string&);

    void setInstructions(std::vector<std::shared_ptr<AbstractCommand>>& in);
    void setCompSuccess(const std::shared_ptr<CFGNode>& compSuccess);
    void setCompFail(const std::shared_ptr<CFGNode>& compFail);
    void setComp(const std::shared_ptr<JumpOnComparisonCommand>& comp);
    void setParentFunction(FunctionSymbol* pf);
    void setLast(bool last = true);

    void addPushingState(const std::shared_ptr<CFGNode>& cfgn);
    //assumes the pop is handled elsewhere
    void removePushes();
    void removePushingState(const std::string& name);
    void replacePushes(const std::string& rep);
    void prepareToDie();

    //tries to merge with other (if it can fix the jumps so that it can do so)
    //returns true if successful
    bool swallowNode(std::shared_ptr<CFGNode> other);

    std::shared_ptr<JumpOnComparisonCommand> getComp();
    std::unordered_map<std::string, std::shared_ptr<CFGNode>>& getPredecessors();
    std::shared_ptr<CFGNode> getCompSuccess();
    int getJumpline() const;
    std::shared_ptr<CFGNode> getCompFail();
    const std::string &getName() const;
    std::vector<std::shared_ptr<AbstractCommand>>& getInstrs();
    ControlFlowGraph& getParentGraph() const;
    FunctionSymbol* getParentFunction() const;
    void printSource(bool makeState = true, std::string delim = "\n");
    void printDotNode();
    bool isLastNode() const;
    bool isFirstNode() const;
};

#endif
