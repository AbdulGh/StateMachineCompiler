#include <stack>

#include "Compiler.h"

using namespace std;
/*expression tree stuff*/
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

CommOperatorNode::CommOperatorNode(Op o): left(nullptr)
{
    if (o == PLUS || o == MULT)
    {
        op = o;
        setType(COMM);
    }
    else throw "Not commutative";
}

void CommOperatorNode::addNode(ExprNodePointer p)
{
    if (left == nullptr)
    {
        left = p;
        varsRequired = left->getVarsRequired();
    }
    else if (p->getVarsRequired() > left->getVarsRequired())
    {
        right.push_back(left);
        left = p;
        varsRequired = p->getVarsRequired();
    }
    else
    {
        right.push_back(p);
        if (p->getVarsRequired() == left->getVarsRequired()) varsRequired = left->getVarsRequired() + 1;
    }
}

NotCommOperatorNode::NotCommOperatorNode(Op o): left(nullptr)
{
    if (o == MINUS || o == DIV || o == MULT)
    {
        op = o;
        setType(NOTCOMM);
    }
    else throw "Commutative";
}

void NotCommOperatorNode::addNode(ExprNodePointer p)
{
    if (left == nullptr)
    {
        left = p;
        varsRequired = left->getVarsRequired();
    }
    else if (right != nullptr)
    {
        if (right->getType() == LITERAL && p->getType() == LITERAL)
        {
            static_pointer_cast<AtomNode<double>>(right)->combine(static_pointer_cast<AtomNode<double>>(p), op);
        }
        else
        {
            ExprNodePointer newP(new NotCommOperatorNode(op));
            if (p->getVarsRequired() >= right->getVarsRequired())
            {
                newP->addNode(p);
                newP->addNode(left);
                left = newP;
                if (left->getVarsRequired() == right->getVarsRequired()) varsRequired = left->getVarsRequired() + 1;
                else varsRequired = left->getVarsRequired();
            }
            else
            {
                newP->addNode(left);
                newP->addNode(right);
                left = newP;
                varsRequired = left->getVarsRequired();
            }
        }
    }
}

template <>
AtomNode<string>::AtomNode(string in)
{
    varsRequired = 0;
    data = in;
    type = IDENTIFIER;
}

template <>
AtomNode<double>::AtomNode(double in)
{
    varsRequired = 0;
    data = in;
    type = LITERAL;
}

template <typename T>
AtomNode<T>::AtomNode(T)
{
    throw "Strange use of template";
}

template <typename T>
void AtomNode<T>::combine(shared_ptr<AtomNode<double>> p, Op o)
{
    if (getType() != LITERAL || p->getType() != LITERAL) throw "Cannot combine w/ identifier";
    switch(o)
    {
        case PLUS:
            data += p->data;
            break;
        case MINUS:
            data -= p->data;
            break;
        case MULT:
            data *= p->data;
            break;
        case DIV:
            data /= p->data;
            break;
        case MOD:
            data =(int)data %  (int)p->data;
            break;
        default: throw "Weird op";
    }
    p.reset();
}

template <typename T>
void AtomNode<T>::addNode(ExprNodePointer)
{
    throw "Atoms have no children";
}

/*generation*/
ExpressionCodeGenerator::ExpressionCodeGenerator(Compiler &p):
        parent(p)
{}

void ExpressionCodeGenerator::CompileExpression(FunctionPointer fs)
{
    ExprNodePointer tree = expression();
}

ExprNodePointer ExpressionCodeGenerator::expression()
{
    ExprNodePointer ttree = term();

    ExprNodePointer currentLeft = term();
    Op lastOp;
    if (!(parent.lookahead.type == OP)) return currentLeft;

    while (parent.lookahead.type == OP && (Op)parent.lookahead.auxType == MULT
                || (Op)parent.lookahead.auxType == DIV || (Op)parent.lookahead.auxType == MOD)
    {
        lastOp = (Op)parent.lookahead.auxType;
        ExprNodePointer ref;
        if (lastOp == MULT) ref.reset(new CommOperatorNode(lastOp));
        else ref.reset(new NotCommOperatorNode(lastOp));
        ref->addNode(currentLeft);

        while ((Op)parent.lookahead.auxType == lastOp)
        {
            parent.match(OP);
            ref->addNode(term());
        }

        currentLeft = ref;
    }
    return currentLeft;
}

ExprNodePointer ExpressionCodeGenerator::term()
{
    ExprNodePointer currentLeft = factor();
    Op lastOp;
    if (!(parent.lookahead.type == OP)) return currentLeft;

    else while (parent.lookahead.type == OP
                && (Op)parent.lookahead.auxType == PLUS || (Op)parent.lookahead.auxType == MINUS)
    {
        lastOp = (Op)parent.lookahead.auxType;
        ExprNodePointer ref;
        if (lastOp == PLUS) ref.reset(new CommOperatorNode(lastOp));
        else ref.reset(new NotCommOperatorNode(lastOp));
        ref->addNode(currentLeft);

        while ((Op)parent.lookahead.auxType == lastOp)
        {
            parent.match(OP);
            ref->addNode(factor());
        }

        currentLeft = ref;
    }
    return currentLeft;
}

ExprNodePointer ExpressionCodeGenerator::factor()
{
    if (parent.lookahead.type == IDENT)
    {
        shared_ptr<Identifier> id = parent.findVariable(parent.ident());
        return shared_ptr<AbstractExprNode>(new AtomNode<string>(id->getUniqueID()));
    }
    else if (parent.lookahead.type == NUMBER)
    {
        double d = stod(parent.lookahead.lexemeString);
        parent.match(NUMBER);
        return shared_ptr<AbstractExprNode>(new AtomNode<double>(d));
    }
    else if (parent.lookahead.type == LPAREN)
    {
        parent.match(LPAREN);
        ExprNodePointer tree = expression();
        parent.match(RPAREN);
        return tree;
    }
    else parent.error("Expected identifier or double in expression");
}