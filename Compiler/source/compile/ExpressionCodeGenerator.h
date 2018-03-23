#ifndef PROJECT_EXPRESSIONTREE_H
#define PROJECT_EXPRESSIONTREE_H

#include <stack>
#include <memory>

#include "Token.h"
#include "Functions.h"


class Compiler;

enum NodeType{COMM, NOTCOMM, LITERAL, IDENTIFIER};

class AbstractExprNode
{
protected:
    NodeType type;
    ArithOp op;
    unsigned int varsRequired = 0;
    void setType(NodeType);

public:
    virtual ~AbstractExprNode() = default;
    virtual void addNode(AbstractExprNode*) = 0;
    virtual const std::unique_ptr<VarWrapper>& getVarWrapper() const {throw std::runtime_error("no var");}
    NodeType getType() const;
    bool isAtom();
    virtual AbstractExprNode* getLeft() = 0;
    virtual AbstractExprNode* getRight() = 0;
    virtual double getDouble() {throw std::runtime_error("not a double lit");}
    unsigned int getVarsRequired() const;

    ArithOp getOp() const;
};

class OperatorNode : public AbstractExprNode
{
private:
    AbstractExprNode* left;
    AbstractExprNode* right;

public:
    AbstractExprNode* getLeft() override;
    AbstractExprNode* getRight() override;

    OperatorNode(ArithOp);
    ~OperatorNode();
    void addNode(AbstractExprNode*) override;
};

class AtomNode : public AbstractExprNode
{
private:
    std::unique_ptr<VarWrapper> data;
    double doub;
    int varsRequired;
public:
    AbstractExprNode* getLeft() override;
    AbstractExprNode* getRight() override;

    AtomNode(std::unique_ptr<VarWrapper>);
    AtomNode(double d);
    double getDouble() override {return doub;}
    const std::unique_ptr<VarWrapper>& getVarWrapper() const override;
    void setData(std::unique_ptr<VarWrapper> vw);
    void setData(double d);
    void addNode(AbstractExprNode*) override;
};

class ExpressionCodeGenerator
{
private:
    static unsigned int nextTemp;
    static unsigned int nextUnique;
    int currentUnique;
    Compiler& parent;
    AbstractExprNode* expression(FunctionSymbol*);
    AbstractExprNode* term(FunctionSymbol*);
    AbstractExprNode* factor(FunctionSymbol*);
    std::unique_ptr<VarWrapper> genTemp(FunctionSymbol*, unsigned int i);
    std::unique_ptr<VarWrapper> genUnique(FunctionSymbol*);
    bool translateTree(AbstractExprNode*, FunctionSymbol*, unsigned int, double&);
    std::unique_ptr<VarWrapper> goingto;
    
public:
    ExpressionCodeGenerator(Compiler& parent, std::unique_ptr<VarWrapper> assignee);
    void compileExpression(FunctionSymbol *fs);
};

#endif
