#ifndef VARIABLE_H
#define VARIABLE_H

#include <forward_list>
#include <unordered_map>
#include <string>
#include <memory>

#include "Enums.h"

class Variable
{
public:
    typedef union DU
    {
        double d;
        std::string* str;

        DU(double doub): d(doub) {};
        DU(std::string* st): str(st) {};

        operator double() const {return d;}
        operator std::string*() const {return str;}
    } DataUnion;

    typedef struct TaggedDataUnion
    {
        Type type;
        DataUnion contents;

        TaggedDataUnion(double doub):
                type(DOUBLE),
                contents(doub) {}

        TaggedDataUnion(std::string st):
                type(STRING),
                contents(new std::string(st)) {}

        TaggedDataUnion(const TaggedDataUnion& o): contents(0.0)
        {
            this->type = o.type;
            if (this->type != STRING) this->contents = o.contents;
            else
            {
                std::string* ref = o.contents;
                this->contents = new std::string(*ref);
            }
        }
    } Data;

    Variable(std::string vname, double vd);
    Variable(std::string vname, std::string str);
    ~Variable();

    const std::string &getName() const;
    Type getType() const;
    const DataUnion &getData() const;
    const TaggedDataUnion &getTaggedDataUnion() const;
    void setData(std::string str);
    void setData(double d);
    void setData(std::shared_ptr<Variable> var);
    void setData(Variable::TaggedDataUnion tdu);

protected:
    std::string name;
    Data data;
};

#endif
