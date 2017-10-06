#ifndef PROJECT_EXPRESSIONTREE_H
#define PROJECT_EXPRESSIONTREE_H

#include <stack>
#include <memory>

#include "Token.h"
#include "Functions.h"


class Compiler;

enum NodeType{COMM, NOTCOMM, LITERAL, IDENTIFIER};

//todo get double rather than casting constantly
class AbstractExprNode
{
protected:
    NodeType type;
    Op op;
    unsigned int varsRequired;
    void setType(NodeType);

public:
    virtual ~AbstractExprNode() = default;
    virtual void addNode(std::shared_ptr<AbstractExprNode>) = 0;
    virtual const std::string& getData() const {throw std::runtime_error("no data");}
    NodeType getType() const;
    bool isAtom();
    virtual std::shared_ptr<AbstractExprNode> getLeft() = 0;
    virtual std::shared_ptr<AbstractExprNode> getRight() = 0;
    virtual double getDouble() {throw std::runtime_error("not a double lit");}
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
    ExprNodePointer getLeft() override;
    ExprNodePointer getRight() override;

    OperatorNode(Op);
    void addNode(ExprNodePointer) override;
};

class AtomNode : public AbstractExprNode
{
private:
    std::string data;
    int varsRequired;
    double doub;
public:
    ExprNodePointer getLeft() override;
    ExprNodePointer getRight() override;

    AtomNode(std::string, bool);
    double getDouble() override {return doub;}
    const std::string& getData() const override;
    void setData(std::string);
    void addNode(ExprNodePointer) override;
};

class ExpressionCodeGenerator
{
private:
    static unsigned int nextTemp;
    static unsigned int nextUnique;
    int currentUnique;
    Compiler& parent;
    ExprNodePointer expression(FunctionSymbol*);
    ExprNodePointer term(FunctionSymbol*);
    ExprNodePointer factor(FunctionSymbol*);
    std::string genTemp(FunctionSymbol*, unsigned int i);
    std::string genUnique(FunctionSymbol*);
    bool translateTree(ExprNodePointer, FunctionSymbol*, unsigned int, double&);
    std::string goingto;
    
public:
    ExpressionCodeGenerator(Compiler& parent, const std::string& assignee);
    void CompileExpression(FunctionSymbol* fs);
};

#endif
