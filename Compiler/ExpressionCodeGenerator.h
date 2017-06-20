#ifndef PROJECT_EXPRESSIONTREE_H
#define PROJECT_EXPRESSIONTREE_H

#include <stack>

#include "Token.h"

typedef std::shared_ptr<FunctionSymbol> FunctionPointer;

class Compiler;

enum NodeType{COMM, NOTCOMM, IDENTIFIER, LITERAL};

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
    virtual std::vector<std::shared_ptr<AbstractExprNode>> getRest() = 0;
    unsigned int getVarsRequired() const;

    Op getOp() const;
};
typedef std::shared_ptr<AbstractExprNode> ExprNodePointer;

class CommOperatorNode : public AbstractExprNode
{
private:
    ExprNodePointer left;
    std::vector<ExprNodePointer> right;

public:
    ExprNodePointer getLeft();
    ExprNodePointer getRight();
    std::vector<ExprNodePointer> getRest();

    CommOperatorNode(Op);
    void addNode(ExprNodePointer);
};

class NotCommOperatorNode : public AbstractExprNode
{
private:
    ExprNodePointer left;
    ExprNodePointer right;

public:
    ExprNodePointer getLeft();
    ExprNodePointer getRight();
    std::vector<ExprNodePointer> getRest();

    NotCommOperatorNode(Op);
    void addNode(ExprNodePointer);
};

template <typename T>
class AtomNode : public AbstractExprNode
{
private:
    T data;

public:
    ExprNodePointer getLeft();
    ExprNodePointer getRight();
    std::vector<ExprNodePointer> getRest();

    AtomNode(T);
    std::string getData();
    void combine(std::shared_ptr<AtomNode<double>>, Op op);
    void addNode(ExprNodePointer);
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
