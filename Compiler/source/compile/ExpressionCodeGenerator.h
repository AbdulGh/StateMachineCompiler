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
    Op op;
    unsigned int varsRequired = 0;
    void setType(NodeType);

public:
    virtual ~AbstractExprNode() = default;
    virtual void addNode(AbstractExprNode*) = 0;
    virtual const std::string& getData() const {throw std::runtime_error("no data");}
    NodeType getType() const;
    bool isAtom();
    virtual AbstractExprNode* getLeft() = 0;
    virtual AbstractExprNode* getRight() = 0;
    virtual double getDouble() {throw std::runtime_error("not a double lit");}
    unsigned int getVarsRequired() const;

    Op getOp() const;
};

class OperatorNode : public AbstractExprNode
{
private:
    AbstractExprNode* left;
    AbstractExprNode* right;

public:
    AbstractExprNode* getLeft() override;
    AbstractExprNode* getRight() override;

    OperatorNode(Op);
    ~OperatorNode();
    void addNode(AbstractExprNode*) override;
};

class AtomNode : public AbstractExprNode
{
private:
    std::string data;
    int varsRequired;
    double doub;
public:
    AbstractExprNode* getLeft() override;
    AbstractExprNode* getRight() override;

    AtomNode(std::string, bool);
    double getDouble() override {return doub;}
    const std::string& getData() const override;
    void setData(std::string);
    void addNode(AbstractExprNode*) override;
};

class ExpressionCodeGenerator //todo make namespace or sth
{
private:
    static unsigned int nextTemp;
    static unsigned int nextUnique;
    int currentUnique;
    Compiler& parent;
    AbstractExprNode* expression(FunctionSymbol*);
    AbstractExprNode* term(FunctionSymbol*);
    AbstractExprNode* factor(FunctionSymbol*);
    std::string genTemp(FunctionSymbol*, unsigned int i);
    std::string genUnique(FunctionSymbol*);
    bool translateTree(AbstractExprNode*, FunctionSymbol*, unsigned int, double&);
    std::string goingto;
    
public:
    ExpressionCodeGenerator(Compiler& parent, const std::string& assignee);
    void compileExpression(FunctionSymbol *fs);
};

#endif
