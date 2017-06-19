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
    unsigned int varsRequired;
    void setType(NodeType);
public:
    virtual void addNode(std::shared_ptr<AbstractExprNode>) = 0;
    NodeType getType() const;
    unsigned int getVarsRequired() const;
};
typedef std::shared_ptr<AbstractExprNode> ExprNodePointer;

class CommOperatorNode : public AbstractExprNode
{
private:
    ExprNodePointer left;
    std::vector<ExprNodePointer> right;
    Op op;

public:
    CommOperatorNode(Op);
    void addNode(ExprNodePointer);
};

class NotCommOperatorNode : public AbstractExprNode
{
private:
    ExprNodePointer left;
    ExprNodePointer right;
    Op op;

public:
    NotCommOperatorNode(Op);
    void addNode(ExprNodePointer);
};

template <typename T>
class AtomNode : public AbstractExprNode
{
private:
    T data;
public:
    AtomNode(T);
    void combine(std::shared_ptr<AtomNode<double>>, Op op);
    void addNode(ExprNodePointer);
};

class ExpressionCodeGenerator
{
private:
    Compiler& parent;
    ExprNodePointer expression();
    ExprNodePointer term();
    ExprNodePointer factor();
    
public:
    ExpressionCodeGenerator(Compiler& parent);
    void CompileExpression(FunctionPointer fs);
};

#endif
