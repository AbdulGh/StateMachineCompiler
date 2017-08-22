#ifndef PROJECT_REPORTER_H
#define PROJECT_REPORTER_H

#include <fstream>

#define NUM_ALERTS 9

class Reporter
{
private:
    std::ofstream output;

    const static std::string enumNames[NUM_ALERTS];
    bool toWarn[NUM_ALERTS];

public:
    enum AlertType {GENERIC, RANGE, UNINITIALISED_USE, UNDECLARED_USE, TYPE,
                    ZERODIVISION, USELESS_OP, BAD_STACK_USE, COMPILER};

    Reporter(std::string filename);
    ~Reporter();
    void setWarning(AlertType type, bool toReport);
    void warn(AlertType type, std::string details);
    void error(AlertType type, std::string details);
};


#endif
