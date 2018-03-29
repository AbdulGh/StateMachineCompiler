#include <stack>

#include "Compiler.h"
#include "../symbolic/VarWrappers.h"
#include "ExpressionCodeGenerator.h"

#include <memory>

using namespace std;

ExpressionCodeGenerator::ExpressionCodeGenerator(Compiler &p, std::unique_ptr<VarWrapper> asignee):
        parent(p),
        currentUnique(0),
        goingto(move(asignee)){}

void ExpressionCodeGenerator::compileExpression(FunctionSymbol *fs)
{
    AbstractExprNode* tree = expression(fs);
    double df;
    if (translateTree(tree, fs,  0, df)) fs->genAssignment(move(goingto), to_string(df), parent.lookahead.line);
    delete tree;
}

AbstractExprNode* ExpressionCodeGenerator::expression(FunctionSymbol* fs)
{
    AbstractExprNode* currentLeft = term(fs);
    if (!(parent.lookahead.type == OP)) return currentLeft;
    else while (parent.lookahead.type == OP
                && ((ArithOp)parent.lookahead.auxType == PLUS || (ArithOp)parent.lookahead.auxType == MINUS))
    {
        ArithOp lastOp = (ArithOp)parent.lookahead.auxType;
        AbstractExprNode* ref = new OperatorNode(lastOp);
        ref->addNode(currentLeft);

        do
        {
            parent.match(OP);
            ref->addNode(term(fs));
        } while (parent.lookahead.type == OP && (ArithOp)parent.lookahead.auxType == lastOp);

        currentLeft = ref;
    }
    return currentLeft;
}

AbstractExprNode* ExpressionCodeGenerator::term(FunctionSymbol* fs)
{
    AbstractExprNode* currentLeft = factor(fs);
    if (!(parent.lookahead.type == OP)) return currentLeft;

    else while (parent.lookahead.type == OP && ((ArithOp)parent.lookahead.auxType == MULT
                || (ArithOp)parent.lookahead.auxType == DIV || (ArithOp)parent.lookahead.auxType == MOD))
    {
        ArithOp lastOp = (ArithOp)parent.lookahead.auxType;
        AbstractExprNode* ref = new OperatorNode(lastOp);
        ref->addNode(currentLeft);

        do
        {
            parent.match(OP);
            ref->addNode(factor(fs));
        } while (parent.lookahead.type == OP && (ArithOp)parent.lookahead.auxType == lastOp);

        currentLeft = ref;
    }
    return currentLeft;
}

AbstractExprNode* ExpressionCodeGenerator::factor(FunctionSymbol* fs)
{
    bool toNegate = false;
    while (parent.lookahead.type == OP && ((ArithOp)parent.lookahead.auxType == MINUS))
    {
        toNegate = !toNegate;
        parent.match(OP);
    }

    auto withNeg = [&, toNegate] (AbstractExprNode* ep) -> AbstractExprNode*
    {
        if (toNegate)
        {
            AbstractExprNode* negateMul = new OperatorNode(MULT);
            negateMul->addNode(ep);
            negateMul->addNode(new AtomNode(-1));
            return negateMul;
        }
        else return ep;
    };

    if (parent.lookahead.type == IDENT) return withNeg(new AtomNode(parent.wrappedIdent()));
    else if (parent.lookahead.type == NUMBER)
    {
        double d = stod(parent.lookahead.lexemeString);
        if (toNegate) d *= -1;
        AbstractExprNode* ref = new AtomNode(d);
        parent.match(NUMBER);
        return ref;
    }
    else if (parent.lookahead.type == LPAREN)
    {
        parent.match(LPAREN);
        AbstractExprNode* tree = expression(fs);
        parent.match(RPAREN);
        return withNeg(tree);
    }
    else if (parent.lookahead.type == CALL)
    {
        parent.genFunctionCall(fs, DOUBLE);
        unique_ptr<VarWrapper> uni = genUnique(fs);
        fs->genAssignment(uni->clone(), make_unique<SVByName>("retD"), parent.lookahead.line);
        return withNeg(new AtomNode(move(uni)));
    }
    else parent.error("Expected identifier or double in expression");
}

unsigned int ExpressionCodeGenerator::nextTemp = 0; //todo quick make next two return SVByName
std::unique_ptr<VarWrapper> ExpressionCodeGenerator::genTemp(FunctionSymbol* fs, unsigned int i)
{
    if (i == 0) return goingto->clone();
    i -= 1;
    if (i == nextTemp)
    {
        string s = "temp" + to_string(nextTemp++);
        fs->genVariableDecl(DOUBLE, s, parent.lookahead.line);
        return make_unique<SVByName>(s);
    }
    if (i > nextTemp) throw "Something went wrong somehow";
    return make_unique<SVByName>("temp" + to_string(i));
}

unsigned int ExpressionCodeGenerator::nextUnique = 0;
std::unique_ptr<VarWrapper> ExpressionCodeGenerator::genUnique(FunctionSymbol* fs)
{
    if (currentUnique == nextUnique)
    {
        string s = "unique" + to_string(nextUnique++);
        ++currentUnique;

        parent.cfg.getFirst()->getInstrs().push_back(make_unique<DeclareVarCommand>(DOUBLE, s, -1));
        return make_unique<SVByName>(s);
    }
    else if (currentUnique > nextUnique) throw "Something went wrong somehow";
    return make_unique<SVByName>("unique" + to_string(currentUnique++));
}

bool ExpressionCodeGenerator::translateTree(AbstractExprNode* p, FunctionSymbol* fs, unsigned int reg, double& ret)
{
    if (p->isAtom())
    {
        if (p->getType() == LITERAL)
        {
            ret = p->getDouble();
            return true;
        }
        else
        {
            fs->genAssignment(genTemp(fs, reg), p->getVarWrapper()->clone(), parent.lookahead.line);
            return false;
        }
    }

    AbstractExprNode* leftp = p->getLeft();
    AbstractExprNode* rightp = p->getRight();
    double dl, dr;
    std::unique_ptr<VarWrapper> left, right;
    bool leftlit = leftp->getType() == LITERAL && !leftp->getVarWrapper();
    bool rightlit = rightp->getType() == LITERAL && !rightp->getVarWrapper();

    if (leftlit) dl = leftp->getDouble();
    if (rightlit) dr = rightp->getDouble();

    if (!leftlit)
    {
        if (leftp->isAtom()) left = leftp->getVarWrapper()->clone();
        else
        {
            leftlit = translateTree(leftp, fs, reg, dl);
            if (!leftlit) left = genTemp(fs, reg); //todo VarWrapper -> term
        }
    }

    if (!rightlit)
    {
        if (rightp->isAtom()) right = rightp->getVarWrapper()->clone();
        else
        {
            rightlit = translateTree(rightp, fs, reg + 1, dr);
            if (!rightlit) right = genTemp(fs, reg + 1);
        }
    }

    if (leftlit && rightlit)
    {
        ret = evaluateOp(dl, p->getOp(), dr);
        return true;
    }
    else
    {
        if (leftlit)
        {
            auto tl = Term(dl);
            auto tr = Term(move(right));
            fs->genExpr(genTemp(fs, reg), tl, p->getOp(), tr, parent.lookahead.line);
        }
        else if (rightlit)
        {
            auto tl = Term(move(left));
            auto tr = Term(dr);
            fs->genExpr(genTemp(fs, reg), tl, p->getOp(), tr, parent.lookahead.line);
        }
        else
        {
            auto tl = Term(move(left));
            auto tr = Term(move(right));
            fs->genExpr(genTemp(fs, reg), tl, p->getOp(), tr, parent.lookahead.line);
        }
        return false;
    }
}

