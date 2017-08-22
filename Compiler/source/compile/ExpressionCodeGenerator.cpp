#include <stack>

#include "Compiler.h"

//todo unary minus

using namespace std;

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
        if (!id->isDefined()) parent.warning(id->getLexeme() + " may not be defined");
        return make_shared<AtomNode>(id->getUniqueID(), false);
    }
    else if (parent.lookahead.type == NUMBER)
    {
        ExprNodePointer ref = make_shared<AtomNode>(parent.lookahead.lexemeString, true);
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
        fs->genAssignment(uni, "retD", parent.lookahead.line);
        return make_shared<AtomNode>(uni, false);
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
        fs->genVariableDecl(DOUBLE, s, parent.lookahead.line);
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
        fs->genVariableDecl(DOUBLE, s, parent.lookahead.line);
        return s;
    }
    else if (currentUnique > nextUnique) throw "Something went wrong somehow";
    return "unique" + to_string(currentUnique++);
}

void ExpressionCodeGenerator::translateTree(ExprNodePointer p, FunctionPointer fs, unsigned int reg)
{
    if (p == nullptr) return;
    if (p->isAtom()) fs->genAssignment(genTemp(fs, reg), static_pointer_cast<AtomNode>(p)->getData(), parent.lookahead.line);
    else
    {
        string thisone = genTemp(fs, reg);
        string left, right;
        if (p->getLeft()->isAtom()) left = static_pointer_cast<AtomNode>(p->getLeft())->getData();
        else
        {
            translateTree(p->getLeft(), fs, reg);
            left = thisone;
        }
        if (p->getRight()->isAtom()) right = static_pointer_cast<AtomNode>(p->getRight())->getData();
        else
        {
            translateTree(p->getRight(), fs, reg+1);
            right = genTemp(fs, reg + 1);
        }

        fs->genExpr(thisone, left, p->getOp(), right, parent.lookahead.line);
    }
}

