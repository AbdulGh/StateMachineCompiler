#include <iostream>

#include "Command.h"

using namespace std;

AbstractCommand::Type AbstractCommand::getType() const
{
    return type;
}

int AbstractCommand::getNextState() const
{
    return nextState;
}

bool AbstractCommand::changesState() const
{
    return changeState;
}


/*PrintConstCommand*/
PrintConstCommand::PrintConstCommand(string istr)
{
    str = istr;
    type = PRINT;
}

void PrintConstCommand::execute(ScopeManager sm)
{
    cout << str;
}

/*PrintVarCommand*/
PrintVarCommand::PrintVarCommand(std::string name)
{
    varN = name;
    type = PRINT;
    //varType = sm.findVariable(varN)->getType();

}

void PrintVarCommand::execute(ScopeManager sm)
{
    shared_ptr<Variable> var = sm.findVariable(varN);
    if (var->getType() == Variable::Type::DOUBLE) {} // todo
}

/*JumpCommand*/
JumpCommand::JumpCommand(int state)
{
    type = JUMP;
    changeState = true;
    nextState = state;
}

void JumpCommand::execute(ScopeManager){}