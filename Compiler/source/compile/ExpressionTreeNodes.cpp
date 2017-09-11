#include "ExpressionCodeGenerator.h"

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

Op AbstractExprNode::getOp() const
{
    return op;
}

OperatorNode::OperatorNode(Op o): left(nullptr), right(nullptr)
{
    op = o;
    if (op == PLUS || op == MULT) setType(COMM);
    else setType(NOTCOMM);
}

void OperatorNode::addNode(ExprNodePointer p)
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
                ExprNodePointer newP = make_shared<OperatorNode>(op);
                newP->addNode(left);
                newP->addNode(right);
                left = newP;
                right = p;
            }
            else
            {
                ExprNodePointer newP = make_shared<OperatorNode>(op);
                newP->addNode(left);
                newP->addNode(p);
                left = newP;
            }

            if (left->getVarsRequired() < right->getVarsRequired()) throw "Disaster";
            else if (left->getVarsRequired() == right->getVarsRequired()) varsRequired = left->getVarsRequired() + 1;
            else varsRequired = left->getVarsRequired();
        }
    }
    else
    {
        if (right == nullptr) right = p;
        else
        {
            ExprNodePointer newP = make_shared<OperatorNode>(op);
            newP->addNode(left);
            newP->addNode(right);
            left = newP;
            right = p;
        }
    }
}

ExprNodePointer OperatorNode::getLeft()
{
    return left;
}

ExprNodePointer OperatorNode::getRight()
{
    return right;
}

AtomNode::AtomNode(string in, bool num):
        varsRequired(0),
        data(in)
{
    setType(num? LITERAL : IDENTIFIER);
}

ExprNodePointer AtomNode::getLeft()
{
    return nullptr;
}

ExprNodePointer AtomNode::getRight()
{
    return nullptr;
}

void AtomNode::addNode(ExprNodePointer)
{
    throw "Atoms have no children";
}

const string& AtomNode::getData() const
{
    return data;
}

void AtomNode::setData(std::string s)
{
    data = s;
}
