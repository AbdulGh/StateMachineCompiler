#ifndef PROJECT_EXPRESSIONTREE_H
#define PROJECT_EXPRESSIONTREE_H

#include <stack>
#include <memory>

#include "Token.h"
#include "FunctionCodeGen.h"

typedef std::shared_ptr<FunctionCodeGen> FunctionPointer;

class Compiler;

enum NodeType{COMM, NOTCOMM, ATOM};

class AbstractExprNode
{
protected:
    NodeType type;
    Op op;
    unsigned int varsRequired;
    void setType(NodeType);

public:
    virtual void addNode(std::shared_ptr<AbstractExprNode>) = 0;
    NodeType getType() const;
    virtual std::shared_ptr<AbstractExprNode> getLeft() = 0;
    virtual std::shared_ptr<AbstractExprNode> getRight() = 0;
    unsigned int getVarsRequired() const;

    Op getOp() const;
};
typedef std::shared_ptr<AbstractExprNode> ExprNodePointer;

class OperatorNode : public AbstractExprNode
{
private:
    ExprNodePointer left;
    ExprNodePointer right;

public:
    ExprNodePointer getLeft();
    ExprNodePointer getRight();

    OperatorNode(Op);
    void addNode(ExprNodePointer);
};

class AtomNode : public AbstractExprNode
{
private:
    std::string data;
    bool isNum;
    int varsRequired;
public:
    ExprNodePointer getLeft();
    ExprNodePointer getRight();

    AtomNode(std::string, bool);
    const std::string getData() const;
    void addNode(ExprNodePointer);

    bool isNumber() const;
};

class ExpressionCodeGenerator
{
private:
    static unsigned int nextTemp;
    static unsigned int nextUnique;
    int currentUnique;
    Compiler& parent;
    ExprNodePointer expression(FunctionPointer);
    ExprNodePointer term(FunctionPointer);
    ExprNodePointer factor(FunctionPointer);
    std::string genTemp(FunctionPointer, unsigned int i);
    std::string genUnique(FunctionPointer);
    void translateTree(ExprNodePointer, FunctionPointer, unsigned int);
    const std::string& goingto;
    
public:
    ExpressionCodeGenerator(Compiler& parent, const std::string& assignee);
    void CompileExpression(FunctionPointer fs);
};

#endif
