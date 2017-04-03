#ifndef SCOPEMANAGER_H
#define SCOPEMANAGER_H

#include <forward_list>
#include <unordered_map>
#include <string>
#include <memory>

class Variable
{
public:
    enum Type {INT, DOUBLE, STRING};
    typedef union DU
    {
        int i;
        double d;
        char* str;

        DU(int in): i(in) {};
        DU(double doub): d(doub) {};
        DU(char* st): str(st) {};

        operator int() const {return i;}
        operator double() const {return d;}
        operator char*() const {return str;}
    } DataUnion;

    Variable(std::string vname, int vi):
            name(vname),
            data(vi),
            type(INT) {}

    Variable(std::string vname, double vd):
            name(vname),
            data(vd),
            type(DOUBLE) {}

    Variable(std::string vname, char* st):
            name(vname),
            data(st),
            type(STRING) {}

    const std::string &getName() const
    {
        return name;
    }

    Type getType() const
    {
        return type;
    }

    const DataUnion &getData() const
    {
        return data;
    }

    void setData(int i)
    {
        if (type != INT) throw std::runtime_error("Cannot assign INT to type " + type);
        Variable::data = i;
    }

    void setData(std::string str)
    {
        if (type != STRING) throw std::runtime_error("Cannot assign STRING to type " + type);
        Variable::data = &str[0u];
    }

    void setData(double d)
    {
        if (type != INT) throw std::runtime_error("Cannot assign DOUBLE to type " + type);
        Variable::data = d;
    }

private:
    std::string name;
    Type type;
    DataUnion data;
};

class ScopeManager
{
private:
    std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Variable>>> currentMap;
    std::forward_list<std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Variable>>>> sTable;

public:
    ScopeManager();
    void pushScope();
    void popScope();
    void declare(std::string name, Variable::Type type);
    std::shared_ptr<Variable> findVariable(std::string name);
    void set(std::string name, int d);
    void set(std::string name, std::string d);
    void set(std::string name, double d);
    //bool isDeclared(std::string name, Variable::Type effect);
    //bool isDefined(std::string name, Variable::Type effect);
};


#endif
