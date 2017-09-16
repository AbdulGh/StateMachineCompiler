#ifndef PROJECT_CFG_H
#define PROJECT_CFG_H

#include <unordered_map>

#include "../compile/SymbolTable.h"
#include "../compile/Token.h"
#include "../Command.h"

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
public:
    void addNode(std::string name, std::vector<std::shared_ptr<AbstractCommand>>& instrs);
    void removeNode(std::string name);
    std::shared_ptr<CFGNode> getNode(std::string, bool create = true);
    std::unordered_map<std::string, std::shared_ptr<CFGNode>>& getCurrentNodes();
    std::string getSource();
    std::shared_ptr<CFGNode> getFirst() const;
    std::shared_ptr<CFGNode> getLast() const;
    void setLast(std::string lastName);

};

class CFGNode: public std::enable_shared_from_this<CFGNode>
{
private:
    std::string name;
    std::shared_ptr<JumpOnComparisonCommand> comp;
    std::unordered_map<std::string, std::shared_ptr<CFGNode>> predecessors;
    std::shared_ptr<CFGNode> compSuccess;
    std::shared_ptr<CFGNode> compFail; //unconditional jump at the end of the node
    std::vector<std::shared_ptr<AbstractCommand>> instrs;
    ControlFlowGraph& parent;
    int jumpline;

public:
    CFGNode(ControlFlowGraph& p, std::string n):
            parent(p), name(n), comp{}, compSuccess{}, compFail{}, jumpline(-1) {}
    void constProp();
    bool addParent(std::shared_ptr<CFGNode>); //returns false if parent was already in
    void removeParent(std::shared_ptr<CFGNode>);
    void removeParent(const std::string&);
    void setInstructions(std::vector<std::shared_ptr<AbstractCommand>> &in);
    const std::string &getName() const;
    void setCompSuccess(const std::shared_ptr<CFGNode> &compSuccess);
    void setCompFail(const std::shared_ptr<CFGNode> &compFail);
    void setComp(const std::shared_ptr<JumpOnComparisonCommand> &comp);
    //tries to merge with other (if it can fix the jumps so that it can do so)
    //returns true if successful
    bool swallowNode (std::shared_ptr<CFGNode> other);
    std::shared_ptr<JumpOnComparisonCommand> getComp();
    std::unordered_map<std::string, std::shared_ptr<CFGNode>>& getPredecessors();
    std::shared_ptr<CFGNode> getCompSuccess();
    int getJumpline() const;
    std::shared_ptr<CFGNode> getCompFail();
    std::vector<std::shared_ptr<AbstractCommand>> &getInstrs();
    ControlFlowGraph &getParent() const;
    std::string getSource();
};

#endif
