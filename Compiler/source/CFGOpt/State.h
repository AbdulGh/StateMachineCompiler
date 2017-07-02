#ifndef STATE_H
#define STATE_H

#include <string>
#include <vector>
#include <memory>

#include "../Command.h"

class State
{
private:
    std::string name;
    std::vector<std::shared_ptr<AbstractCommand>> instructions;

public:
    std::vector<std::shared_ptr<AbstractCommand>> &getInstructions();
    const std::string &getName() const;
    void setInstructions(const std::vector<std::shared_ptr<AbstractCommand>> &instructions);

    State(std::string);
    //~State();
};


#endif
