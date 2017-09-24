#include <stack>

#include "Compiler.h"

using namespace std;

ExpressionCodeGenerator::ExpressionCodeGenerator(Compiler &p, const string& asignee):
        parent(p),
        currentUnique(0),
        goingto(asignee){}

void ExpressionCodeGenerator::CompileExpression(FunctionPointer fs)
{
    ExprNodePointer tree = expression(fs);
    double df;
    if (translateTree(tree, fs,  0, df)) fs->genAssignment(goingto, to_string(df), parent.lookahead.line);
}

ExprNodePointer ExpressionCodeGenerator::expression(FunctionPointer fs)
{
    ExprNodePointer currentLeft = term(fs);
    if (!(parent.lookahead.type == OP)) return currentLeft;
    else while (parent.lookahead.type == OP
                && ((Op)parent.lookahead.auxType == PLUS || (Op)parent.lookahead.auxType == MINUS))
    {
        Op lastOp = (Op)parent.lookahead.auxType;
        ExprNodePointer ref = make_shared<OperatorNode>(lastOp);
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
        ExprNodePointer ref = make_shared<OperatorNode>(lastOp);
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
    bool toNegate = false;
    while (parent.lookahead.type == OP && ((Op)parent.lookahead.auxType == MINUS))
    {
        toNegate = !toNegate;
        parent.match(OP);
    }

    auto withNeg = [&, toNegate] (ExprNodePointer ep) -> ExprNodePointer
    {
        if (toNegate)
        {
            ExprNodePointer negateMul = make_shared<OperatorNode>(MULT);
            negateMul->addNode(ep);
            negateMul->addNode(make_shared<AtomNode>("-1", true));
            return negateMul;
        }
        else return ep;
    };

    if (parent.lookahead.type == IDENT)
    {
        shared_ptr<Identifier> id = parent.findVariable(parent.ident());
        if (!id->isDefined()) parent.warning(id->getLexeme() + " may not be defined");
        return withNeg(make_shared<AtomNode>(id->getUniqueID(), false));
    }
    else if (parent.lookahead.type == NUMBER)
    {
        ExprNodePointer ref = make_shared<AtomNode>(toNegate ? "-" +parent.lookahead.lexemeString :
                                                    parent.lookahead.lexemeString, true);
        parent.match(NUMBER);
        return ref;
    }
    else if (parent.lookahead.type == LPAREN)
    {
        parent.match(LPAREN);
        ExprNodePointer tree = expression(fs);
        parent.match(RPAREN);
        return withNeg(tree);
    }
    else if (parent.lookahead.type == CALL)
    {
        parent.genFunctionCall(DOUBLE, fs);
        string uni = genUnique(fs);
        fs->genAssignment(uni, "retD", parent.lookahead.line);
        return withNeg(make_shared<AtomNode>(uni, false));
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

bool ExpressionCodeGenerator::translateTree(ExprNodePointer p, FunctionPointer fs, unsigned int reg, double& ret)
{
    if (p->isAtom())
    {
        if (p->getType() == LITERAL)
        {
            ret = stod(p->getData());
            return true;
        }
        else
        {
            fs->genAssignment(genTemp(fs, reg), p->getData(), parent.lookahead.line);
            return false;
        }
    }

    ExprNodePointer leftp = p->getLeft();
    ExprNodePointer rightp = p->getRight();
    double dl, dr;
    string left, right;
    bool leftlit = leftp->getType() == LITERAL;
    bool rightlit = rightp->getType() == LITERAL;

    if (leftlit) dl = stod(leftp->getData());
    if (rightlit) dr = stod(rightp->getData());

    if (!leftlit || !rightlit)
    {
        if (leftp->isAtom()) left = leftp->getData();
        else if (leftlit = translateTree(leftp, fs, reg, dl)) left = to_string(dl);
        else left = genTemp(fs, reg);
        
        if (rightp->isAtom()) right = rightp->getData();
        else if (rightlit = translateTree(rightp, fs, reg + 1, dr)) right = to_string(dr);
        else right = genTemp(fs, reg + 1);
    }
    if (leftlit && rightlit)
    {
        ret = evaluateOp(dl, p->getOp(), dr);
        return true;
    }
    else
    {
        fs->genExpr(genTemp(fs, reg), left, p->getOp(), right, parent.lookahead.line);
        return false;
    }
}

