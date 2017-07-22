#include "Variable.h"

using namespace std;

Variable::Variable(string vname, double vd):
        name(vname),
        data(vd) {}

Variable::Variable(string vname, string str):
        name(vname),
        data(str) {}

Variable::~Variable()
{
    if (getType() == STRING) delete getData();
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

void Variable::setData(shared_ptr<Variable> var)
{
    setData(var->data);
}

void Variable::setData(Variable::TaggedDataUnion tdu)
{
    if (data.type != tdu.type) throw runtime_error("Invalid assignment to type " + data.type);
    data = tdu;
}