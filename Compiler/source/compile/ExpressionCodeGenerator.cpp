#include <stack>

#include "Compiler.h"
#include "VarWrappers.h"

#include <memory>

using namespace std;

ExpressionCodeGenerator::ExpressionCodeGenerator(Compiler &p, std::unique_ptr<VarSetter> asignee):
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
            negateMul->addNode(new AtomNode("-1", true));
            return negateMul;
        }
        else return ep;
    };

    if (parent.lookahead.type == IDENT)
    {
        unique_ptr<VarGetter> vg = parent.identGetter();
        return withNeg(new AtomNode(vg->getFullName(), false));
    }
    else if (parent.lookahead.type == NUMBER)
    {
        AbstractExprNode* ref = new AtomNode(toNegate ? "-" +parent.lookahead.lexemeString :
                                                    parent.lookahead.lexemeString, true);
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
        unique_ptr<VarSetter> uni = genUnique(fs);
        std::string name = uni->getFullName();
        fs->genAssignment(move(uni), "retD", parent.lookahead.line);
        return withNeg(new AtomNode(name, false));
    }
    else parent.error("Expected identifier or double in expression");
}

unsigned int ExpressionCodeGenerator::nextTemp = 0; //todo quick make next two return SetSVByName
std::unique_ptr<VarSetter> ExpressionCodeGenerator::genTemp(FunctionSymbol* fs, unsigned int i)
{
    if (i == 0) return goingto->clone();
    i -= 1;
    if (i == nextTemp)
    {
        string s = "temp" + to_string(nextTemp++);
        fs->genVariableDecl(DOUBLE, s, parent.lookahead.line);
        return make_unique<SetSVByName>(s);
    }
    if (i > nextTemp) throw "Something went wrong somehow";
    return make_unique<SetSVByName>("temp" + to_string(i));
}

unsigned int ExpressionCodeGenerator::nextUnique = 0;
std::unique_ptr<VarSetter> ExpressionCodeGenerator::genUnique(FunctionSymbol* fs)
{
    if (currentUnique == nextUnique)
    {
        string s = "unique" + to_string(nextUnique++);
        ++currentUnique;
        fs->genVariableDecl(DOUBLE, s, parent.lookahead.line);
        return make_unique<SetSVByName>(s);
    }
    else if (currentUnique > nextUnique) throw "Something went wrong somehow";
    return make_unique<SetSVByName>("unique" + to_string(currentUnique++));
}

bool ExpressionCodeGenerator::translateTree(AbstractExprNode* p, FunctionSymbol* fs, unsigned int reg, double& ret)
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

    AbstractExprNode* leftp = p->getLeft();
    AbstractExprNode* rightp = p->getRight();
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
        else left = genTemp(fs, reg)->getFullName(); //todo varsetter -> term
        
        if (rightp->isAtom()) right = rightp->getData();
        else if (rightlit = translateTree(rightp, fs, reg + 1, dr)) right = to_string(dr);
        else right = genTemp(fs, reg + 1)->getFullName();
    }

    if (leftlit && rightlit)
    {
        ret = evaluateOp(dl, p->getOp(), dr);
        return true;
    }
    else
    {
        auto debug1 = EvaluateExprCommand::Term(left);
        auto debug2 = EvaluateExprCommand::Term(right);
        auto debug3 = p;
        fs->genExpr(genTemp(fs, reg), debug1, p->getOp(), debug2, parent.lookahead.line);
        return false;
    }
}

