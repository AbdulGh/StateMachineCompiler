#include "Variable.h"

using namespace std;

Variable::Variable(string vname, double vd):
        name(move(vname)),
        data(vd) {}

Variable::Variable(string vname, string str):
        name(move(vname)),
        data(move(str)) {}

Variable::~Variable()
{
    if (data.type == STRING) delete data.contents.str;
}

const string &Variable::getName() const
{
    return name;
}

Type Variable::getType() const
{
    return data.type;
}

const Variable::DataUnion &Variable::getData() const
{
    return data.contents;
}

const Variable::TaggedDataUnion &Variable::getTaggedDataUnion() const
{
    return data;
}

void Variable::setData(string str)
{
    if (data.type != STRING) throw runtime_error("Cannot assign STRING to type " + data.type);
    delete getData();
    data.contents = new string(str);
}

void Variable::setData(double d)
{
    if (data.type != DOUBLE) throw runtime_error("Cannot assign DOUBLE to type " + data.type);
    data.contents = d;
}

void Variable::setData(Variable* var)
{
    setData(var->data);
}

void Variable::setData(Variable::TaggedDataUnion tdu)
{
    if (data.type != tdu.type) throw runtime_error("Invalid assignment to type " + data.type);
    data = tdu;
}