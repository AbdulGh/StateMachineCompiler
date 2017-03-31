#ifndef STATE_H
#define STATE_H

#include <string>
#include <vector>

#include "Command.h"

class State
{
private:
    int mnextState = -1;
    std::string name;
    std::vector<std::shared_ptr<AbstractCommand>> instructions;
public:
    const std::vector<std::shared_ptr<AbstractCommand>> &getInstructions() const;
    const std::string &getName() const;
    void setInstructions(const std::vector<std::shared_ptr<AbstractCommand>> &instructions);

    State(std::string);
    void run(ScopeManager);
    int nextState() const;
};


#endif
