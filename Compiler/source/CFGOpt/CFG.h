#ifndef PROJECT_CFG_H
#define PROJECT_CFG_H

#include "State.h"
#include "../compile/SymbolTable.h"
#include "../compile/FunctionCodeGen.h"
#include "../compile/Token.h"


class ControlFlowGraph //todo get this to replace states
{
private:
    struct Comparison
    {
        std::string left;
        bool leftIsConst;
        std::string right;
        bool rightIsConst;
        Relop rel;

        Comparison(std::string, bool, std::string, bool, Relop);
    };

    class CFGNode: public std::enable_shared_from_this<CFGNode>
    {
    private:
        std::string name;
        Comparison comp;
        std::vector<std::shared_ptr<CFGNode>> incoming;
        std::shared_ptr<CFGNode> left;
        std::shared_ptr<CFGNode> right;
        std::vector<std::shared_ptr<AbstractCommand>> instrs;

    public:
        CFGNode(ControlFlowGraph& p, std::string n); //std::vector<std::shared_ptr<AbstractCommand>> instructions);
        void addParent(std::shared_ptr<CFGNode>);
    };

    SymbolTable& symTable;
    std::unordered_map<std::string, FunctionPointer>& funTable;
    std::unordered_map<std::string, std::shared_ptr<CFGNode>> currentNodes;

public:
    void addNode(std::string name, std::vector<std::shared_ptr<AbstractCommand>> instrs);
    ControlFlowGraph(SymbolTable& symbolTable, std::unordered_map<std::string, FunctionPointer>& functionTable):
        symTable(symbolTable), funTable(functionTable) {};
    std::shared_ptr<CFGNode> createNodeIfNotExists(std::string);
};


#endif
