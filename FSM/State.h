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
    int mnextState;
    std::string name;
    std::vector<std::unique_ptr<AbstractCommand>> instructions;
    FSM& parent;

public:
    const std::vector<std::unique_ptr<AbstractCommand>>& getInstructions() const;
    const std::string& getName() const;
    void setInstructions(std::vector<std::unique_ptr<AbstractCommand>> instructions);

    State(std::string, FSM&);
    void run();
    int nextState() const;
};


#endif
