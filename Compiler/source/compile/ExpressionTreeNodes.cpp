#include "ExpressionCodeGenerator.h"
#include "../symbolic/VarWrappers.h"

using namespace std;

NodeType AbstractExprNode::getType() const
{
    return type;
}

void AbstractExprNode::setType(NodeType type)
{
    AbstractExprNode::type = type;
}

unsigned int AbstractExprNode::getVarsRequired() const
{
    return varsRequired;
}

bool AbstractExprNode::isAtom()
{
    return (type == LITERAL || type == IDENTIFIER);
}

ArithOp AbstractExprNode::getOp() const
{
    return op;
}

OperatorNode::OperatorNode(ArithOp o): left{nullptr}, right{nullptr}
{
    op = o;
    if (op == PLUS || op == MULT) setType(COMM);
    else setType(NOTCOMM);
}

OperatorNode::~OperatorNode()
{
    //delete on nullptr does nothing (ie doesnt crash)
    delete left;
    delete right;
}

void OperatorNode::addNode(AbstractExprNode* p)
{
    if (left == nullptr)
    {
        left = p;
        if (left->isAtom()) varsRequired = 1;
        else varsRequired = left->getVarsRequired();
    }
    else if (getType() == COMM && (p->isAtom() || op == p->getOp()))
    {
        if (p->getVarsRequired() > left->getVarsRequired())
        {
            if (right == nullptr)
            {
                right = left;
                left = p;
            }
            else left->addNode(p);
        }
        else
        {
            if (right == nullptr) right = p;
            else if (right->getVarsRequired() > p->getVarsRequired())
            {
                AbstractExprNode* newP = new OperatorNode(op);
                newP->addNode(left);
                newP->addNode(right);
                left = newP;
                right = p;
            }
            else
            {
                AbstractExprNode* newP = new OperatorNode(op);
                newP->addNode(left);
                newP->addNode(p);
                left = newP;
            }

            if (left->getVarsRequired() < right->getVarsRequired()) throw std::runtime_error("Disaster");
            else if (left->getVarsRequired() == right->getVarsRequired()) varsRequired = left->getVarsRequired() + 1;
            else varsRequired = left->getVarsRequired();
        }
    }
    else
    {
        if (right == nullptr) right = p;
        else
        {
            AbstractExprNode* newP = new OperatorNode(op);
            newP->addNode(left);
            newP->addNode(right);
            left = newP;
            right = p;
        }
    }
}

AbstractExprNode* OperatorNode::getLeft()
{
    return left;
}

AbstractExprNode* OperatorNode::getRight()
{
    return right;
}

AtomNode::AtomNode(std::unique_ptr<VarWrapper> vw):
        varsRequired(0), data(move(vw))
{
    setType(IDENTIFIER);
}

AtomNode::AtomNode(double d):
    varsRequired(0), doub(d), data{nullptr}
{
    setType(LITERAL);
}

AbstractExprNode* AtomNode::getLeft()
{
    return nullptr;
}

AbstractExprNode* AtomNode::getRight()
{
    return nullptr;
}

void AtomNode::addNode(AbstractExprNode*)
{
    throw std::runtime_error("Atoms have no children");
}

const unique_ptr<VarWrapper>& AtomNode::getVarWrapper() const
{
    return data;
}

void AtomNode::setData(unique_ptr<VarWrapper> s)
{
    data = move(s);
    setType(IDENTIFIER);
}

void AtomNode::setData(double d)
{
    doub = d;
    setType(LITERAL);
}
