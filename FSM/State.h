#ifndef STATE_H
#define STATE_H

#include <string>
#include <vector>
#include <memory>

class AbstractCommand;
class FSM;
class State
{
private:
    int mnextState = -1;
    std::string name;
    std::vector<std::shared_ptr<AbstractCommand>> instructions;
    FSM& parent;

public:
    const std::vector<std::shared_ptr<AbstractCommand>> &getInstructions() const;
    const std::string &getName() const;
    void setInstructions(const std::vector<std::shared_ptr<AbstractCommand>> &instructions);

    State(std::string, FSM&);
    void run();
    int nextState() const;
};


#endif
