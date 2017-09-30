#ifndef PROJECT_CFG_H
#define PROJECT_CFG_H

#include <unordered_map>

#include "../compile/SymbolTable.h"
#include "../compile/Token.h"
#include "../Command.h"
#include "../compile/Reporter.h"

class FunctionCodeGen;
typedef std::shared_ptr<FunctionCodeGen> FunctionPointer;

//todo use unique pointers here

class CFGNode;
class ControlFlowGraph
{
private:
    std::unordered_map<std::string, std::shared_ptr<CFGNode>> currentNodes;
    std::shared_ptr<CFGNode> first;
    std::shared_ptr<CFGNode> last;
    Reporter& reporter;


public:
    ControlFlowGraph(Reporter& r) : reporter(r) {};
    std::shared_ptr<CFGNode> createNode(const std::string &name, FunctionCodeGen* parent, bool overwrite, bool last);
    std::shared_ptr<CFGNode> getNode(const std::string& name);
    void removeNode(std::string name);
    std::unordered_map<std::string, std::shared_ptr<CFGNode>>& getCurrentNodes();
    std::string getSource();
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
    std::vector<std::shared_ptr<AbstractCommand>> instrs; //todo why pointers here?
    std::vector<std::shared_ptr<CFGNode>> pushingStates; //used to remove pushes if the node is being removed
    std::vector<std::shared_ptr<CFGNode>> returnTo; //todo make this go to the function
    ControlFlowGraph& parentGraph;
    FunctionCodeGen* parentFunction;
    bool isLast;
    int jumpline;

public:
    CFGNode(ControlFlowGraph& p, FunctionCodeGen* pf, std::string n, bool last = false);
    bool constProp(); //returns true if it bypassed some return
    bool addParent(std::shared_ptr<CFGNode>); //returns false if parent was already in
    void removeParent(std::shared_ptr<CFGNode>);
    void removeParent(const std::string&);

    void setInstructions(std::vector<std::shared_ptr<AbstractCommand>>& in);
    void setCompSuccess(const std::shared_ptr<CFGNode>& compSuccess);
    void setCompFail(const std::shared_ptr<CFGNode>& compFail);
    void setComp(const std::shared_ptr<JumpOnComparisonCommand>& comp);
    void setParentFunction(FunctionCodeGen* pf);
    void setLast(bool last = true);

    void addReturnSuccessor(const std::shared_ptr<CFGNode>& other);
    void addReturnSuccessors(const std::vector<std::shared_ptr<CFGNode>>& newRet);
    void clearReturnSuccessors();
    void setReturnSuccessors(std::vector<std::shared_ptr<CFGNode>>& newRet);
    void addPushingState(const std::shared_ptr<CFGNode>& cfgn);
    void removePushes(); //assumes the pop is handled elsewhere

    //tries to merge with other (if it can fix the jumps so that it can do so)
    //returns true if successful
    bool swallowNode(std::shared_ptr<CFGNode> other);

    std::shared_ptr<JumpOnComparisonCommand> getComp();
    std::unordered_map<std::string, std::shared_ptr<CFGNode>>& getPredecessors();
    std::vector<std::shared_ptr<CFGNode>>& getReturnSuccessors();
    std::shared_ptr<CFGNode> getCompSuccess();
    int getJumpline() const;
    std::shared_ptr<CFGNode> getCompFail();
    const std::string &getName() const;
    std::vector<std::shared_ptr<AbstractCommand>>& getInstrs();
    ControlFlowGraph& getParentGraph() const;
    FunctionCodeGen* getParentFunction() const;
    std::string getSource();
    bool isLastNode() const;
};

#endif
