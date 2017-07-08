#ifndef PROJECT_CFG_H
#define PROJECT_CFG_H

#include "../compile/SymbolTable.h"
#include "../compile/Token.h"
#include "../Command.h"

class FunctionCodeGen;
typedef std::shared_ptr<FunctionCodeGen> FunctionPointer;


class CFGNode;
class ControlFlowGraph //todo get this to replace states
{
private:
    //SymbolTable& symTable;
    //std::unordered_map<std::string, FunctionPointer>& funTable;
    std::unordered_map<std::string, std::shared_ptr<CFGNode>> currentNodes;
    std::shared_ptr<CFGNode> first;
public:
    void addNode(std::string name, std::vector<std::shared_ptr<AbstractCommand>> instrs);
    void removeNode(std::string name);
    std::shared_ptr<CFGNode> createNodeIfNotExists(std::string);
    std::unordered_map<std::string, std::shared_ptr<CFGNode>> &getCurrentNodes();
    std::string getSource();
};

class CFGNode: public std::enable_shared_from_this<CFGNode>
{
private:
    std::string name;
    std::shared_ptr<JumpOnComparisonCommand> comp;
    std::vector<std::shared_ptr<CFGNode>> predecessors;
    std::shared_ptr<CFGNode> compSuccess;
public:
    void setCompSuccess(const std::shared_ptr<CFGNode> &compSuccess);

    void setCompFail(const std::shared_ptr<CFGNode> &compFail);

private:
    std::shared_ptr<CFGNode> compFail; //unconditional jump at the end of the node
    std::vector<std::shared_ptr<AbstractCommand>> instrs;
    ControlFlowGraph& parent;

public:
    CFGNode(ControlFlowGraph& p, std::string n):
            parent(p), name(n), comp{}, compSuccess{}, compFail{} {}
    void addParent(std::shared_ptr<CFGNode>);
    void removeParent(std::shared_ptr<CFGNode>);
    void setInstructions(const std::vector<std::shared_ptr<AbstractCommand>> &in);
    const std::string &getName() const;
    std::shared_ptr<JumpOnComparisonCommand> &getComp();
    std::vector<std::shared_ptr<CFGNode>> &getPredecessors();
    std::shared_ptr<CFGNode> getCompSuccess();
    std::shared_ptr<CFGNode> getCompFail();
    std::vector<std::shared_ptr<AbstractCommand>> &getInstrs();
    std::string getSource();

};
/*struct Comparison
    {
        std::string left;
        std::string right;
        Relop rel;

        Comparison(std::string left, std::string right, Relop r):
                left(left),
                right(right),
                rel(r) {}
    };*/


#endif
