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

Op AbstractExprNode::getOp() const
{
    return op;
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

ExprNodePointer CommOperatorNode::getLeft()
{
    return left;
}

ExprNodePointer CommOperatorNode::getRight()
{
    throw "No single right node";
}

vector<ExprNodePointer> CommOperatorNode::getRest()
{
    return right;
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

void NotCommOperatorNode::addNode(ExprNodePointer p) //todo refine this w/ associativity
{
    if (left == nullptr)
    {
        left = p;
        varsRequired = left->getVarsRequired();
    }
    else if (right != nullptr)
    {
        ExprNodePointer newP(new NotCommOperatorNode(op));
        newP->addNode(this->left);
        newP->addNode(this->right);
        this->left = newP;
        this->right = p;
    }
}

ExprNodePointer NotCommOperatorNode::getLeft()
{
    return left;
}

ExprNodePointer NotCommOperatorNode::getRight()
{
    return right;
}

vector<ExprNodePointer> NotCommOperatorNode::getRest()
{
    throw "Only two nodes";
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
ExprNodePointer AtomNode<T>::getLeft()
{
    return nullptr;
}

template <typename T>
ExprNodePointer AtomNode<T>::getRight()
{
    return nullptr;
}

template <typename T>
vector<ExprNodePointer> AtomNode<T>::getRest()
{
    throw "nothing here";
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

template <typename T>
string AtomNode<T>::getData()
{
    return to_string(data);
}

/*generation*/
ExpressionCodeGenerator::ExpressionCodeGenerator(Compiler &p, const string& asignee):
        parent(p),
        currentUnique(0),
        goingto(asignee){}

void ExpressionCodeGenerator::CompileExpression(FunctionPointer fs)
{
    ExprNodePointer tree = expression(fs);
    translateTree(tree, fs,  0);
}

ExprNodePointer ExpressionCodeGenerator::expression(FunctionPointer fs)
{
    ExprNodePointer currentLeft = term(fs);
    Op lastOp;
    if (!(parent.lookahead.type == OP)) return currentLeft;

    else while (parent.lookahead.type == OP
                && ((Op)parent.lookahead.auxType == PLUS || (Op)parent.lookahead.auxType == MINUS))
        {
            lastOp = (Op)parent.lookahead.auxType;
            ExprNodePointer ref;
            if (lastOp == PLUS) ref.reset(new CommOperatorNode(lastOp));
            else ref.reset(new NotCommOperatorNode(lastOp));
            ref->addNode(currentLeft);

            do
            {
                parent.match(OP);
                ref->addNode(term(fs));
            } while (parent.lookahead.type == OP && (Op)parent.lookahead.auxType == lastOp);

            currentLeft = ref;
        }
    return currentLeft;
}

ExprNodePointer ExpressionCodeGenerator::term(FunctionPointer fs)
{
    ExprNodePointer currentLeft = factor(fs);
    Op lastOp;
    if (!(parent.lookahead.type == OP)) return currentLeft;

    while (parent.lookahead.type == OP && ((Op)parent.lookahead.auxType == MULT
                || (Op)parent.lookahead.auxType == DIV || (Op)parent.lookahead.auxType == MOD))
    {
        lastOp = (Op)parent.lookahead.auxType;
        ExprNodePointer ref;
        if (lastOp == MULT) ref.reset(new CommOperatorNode(lastOp));
        else ref.reset(new NotCommOperatorNode(lastOp));
        ref->addNode(currentLeft);

        do
        {
            parent.match(OP);
            ref->addNode(factor(fs));
        } while (parent.lookahead.type == OP && (Op)parent.lookahead.auxType == lastOp);

        currentLeft = ref;
    }
    return currentLeft;
}

ExprNodePointer ExpressionCodeGenerator::factor(FunctionPointer fs)
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
        ExprNodePointer tree = expression(fs);
        parent.match(RPAREN);
        return tree;
    }
    else if (parent.lookahead.type == CALL)
    {
        parent.genFunctionCall(DOUBLE, fs);
        string uni = genUnique(fs);
        fs->emit(uni + " = retD;\n");
        return shared_ptr<AbstractExprNode>(new AtomNode<string>(uni));
    }
    else parent.error("Expected identifier or double in expression");
}

unsigned int ExpressionCodeGenerator::nextTemp = 0;
string ExpressionCodeGenerator::genTemp(FunctionPointer fs, unsigned int i)
{
    if (i == 0) return goingto;
    i -= 1;
    if (i == nextTemp)
    {
        string s = "temp" + to_string(nextTemp++);
        fs->emit("double " + s + ";\n");
        return s;
    }
    if (i > nextTemp) throw "Something went wrong somehow";
    return "temp" + to_string(i);
}

unsigned int ExpressionCodeGenerator::nextUnique = 0;
string ExpressionCodeGenerator::genUnique(FunctionPointer fs)
{
    if (currentUnique == nextUnique)
    {
        string s = "unique" + to_string(nextUnique++);
        currentUnique++;
        fs->emit("double " + s + ";\n");
        return s;
    }
    if (currentUnique > nextUnique) throw "Something went wrong somehow";
    currentUnique++;
    return "unique" + to_string(currentUnique);
}

void ExpressionCodeGenerator::translateTree(ExprNodePointer p, FunctionPointer fs, unsigned int reg)
{
    if (p == nullptr)
    {
        return;
    }
    string thisone = genTemp(fs, reg);
    if (p->getType() == LITERAL || p->getType() == IDENTIFIER)
    {
        fs->emit(thisone + " = " + static_pointer_cast<AtomNode<double>>(p)->getData() + ";\n");
    }
    else if (p->getType() == COMM)
    {
        string nextone = genTemp(fs, reg + 1);
        translateTree(p->getLeft(), fs, reg);
        vector<ExprNodePointer> list = p->getRest();
        string c;
        switch (p->getOp())
        {
            case PLUS:
                c = " + ";
                break;
            case MULT:
                c = " * ";
                break;
            default:
                throw "Impostor commutative op";
        }
        for (auto it = list.cbegin(); it != list.cend(); ++it)
        {
            translateTree(*it, fs, reg + 1);
            fs->emit(thisone + " = " + thisone + c + nextone + ";\n");
        }
    }

    else if (p->getType() == NOTCOMM)
    {
        string nextone = genTemp(fs, reg + 1);
        translateTree(p->getLeft(), fs, reg);
        translateTree(p->getRight(), fs, reg + 1);

        string c;
        switch (p->getOp())
        {
            case MINUS:
                c = " - ";
                break;
            case DIV:
                c = " / ";
                break;
            case MOD:
                c = " % ";
                break;
            default:
                throw "Impostor not-commutative op";
        }
        fs->emit(thisone + " = " + thisone + c + nextone);
    }
}