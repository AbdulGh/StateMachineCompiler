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
        if (left->getType() == ATOM) varsRequired = 1;
        else varsRequired = left->getVarsRequired();
    }
    else if (getType() == COMM && (p->getType() == ATOM || op == p->getOp()))
    {
        if (p->getVarsRequired() > left->getVarsRequired())
        {
            left->addNode(p);
            varsRequired = left->getVarsRequired();
        }
        else
        {
            if (right == nullptr) right = p;
            else if (right->getVarsRequired() > p->getVarsRequired())
            {
                ExprNodePointer newP(new OperatorNode(op));
                newP->addNode(left);
                newP->addNode(right);
                left = newP;
                right = p;
            }
            else
            {
                ExprNodePointer newP(new OperatorNode(op));
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
            ExprNodePointer newP(new OperatorNode(op));
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
    data(in),
    isNum(num)
{
    setType(ATOM);
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

const string AtomNode::getData() const
{
    return data;
}

bool AtomNode::isNumber() const
{
    return isNum;
}

/*generation*/
ExpressionCodeGenerator::ExpressionCodeGenerator(Compiler &p, const string& asignee):
        parent(p),
        currentUnique(0),
        goingto(asignee){}

string printTree(ExprNodePointer p) //debug
{
    string s = "";
    if (p == nullptr) s = "null";
    else if (p->getType() == ATOM) s += static_pointer_cast<AtomNode>(p)->getData();
    else
    {
        s += "[";
        switch (p->getOp())
        {
            case PLUS:
                s += " + ";
                break;
            case MULT:
                s += " * ";
                break;
            case MINUS:
                s += " - ";
                break;
            case DIV:
                s += " / ";
                break;
            case MOD:
                s += " % ";
                break;
            default:
                throw "Strange op";
        }
        s += "(" + to_string(p->getVarsRequired()) + ") ";
        s += printTree(p->getLeft()) + " " + printTree(p->getRight()) + " ]";
    }
    return s;
}

void ExpressionCodeGenerator::CompileExpression(FunctionPointer fs)
{
    ExprNodePointer tree = expression(fs);
    printf(printTree(tree).c_str());
    printf("\n");
    translateTree(tree, fs,  0);
}

ExprNodePointer ExpressionCodeGenerator::expression(FunctionPointer fs)
{
    ExprNodePointer currentLeft = term(fs);
    if (!(parent.lookahead.type == OP)) return currentLeft;
    else while (parent.lookahead.type == OP
                && ((Op)parent.lookahead.auxType == PLUS || (Op)parent.lookahead.auxType == MINUS))
    {
        ExprNodePointer ref;
        Op lastOp = (Op)parent.lookahead.auxType;
        ref.reset(new OperatorNode(lastOp));
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
    if (!(parent.lookahead.type == OP)) return currentLeft;

    else while (parent.lookahead.type == OP && ((Op)parent.lookahead.auxType == MULT
                || (Op)parent.lookahead.auxType == DIV || (Op)parent.lookahead.auxType == MOD))
    {
        Op lastOp = (Op)parent.lookahead.auxType;
        ExprNodePointer ref;
        ref.reset(new OperatorNode(lastOp));
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
        //const char* dbg = id->getUniqueID().c_str();
        return ExprNodePointer(new AtomNode(id->getUniqueID(), false));
    }
    else if (parent.lookahead.type == NUMBER)
    {
        ExprNodePointer ref(new AtomNode(parent.lookahead.lexemeString, true));
        parent.match(NUMBER);
        return ref;
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
        return shared_ptr<AbstractExprNode>(new AtomNode(uni, false));
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
    if (p->getType() == ATOM)
    {
        fs->emit(thisone + " = " + static_pointer_cast<AtomNode>(p)->getData() + ";\n");
    }
    else
    {
        string nextone;
        translateTree(p->getLeft(), fs, reg);
        if (p->getRight()->getType() == ATOM) nextone = static_pointer_cast<AtomNode>(p->getRight())->getData();
        else
        {
            translateTree(p->getRight(), fs, reg+1);
            nextone = genTemp(fs, reg + 1);
        }

        string c;
        switch (p->getOp())
        {
            case PLUS:
                c = " + ";
                break;
            case MULT:
                c = " * ";
                break;
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
                throw "Strange op";
        }
        fs->emit(thisone + " = " + thisone + c + nextone + ";\n");
    }
}

