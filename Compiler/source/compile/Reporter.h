#ifndef PROJECT_REPORTER_H
#define PROJECT_REPORTER_H

#include <fstream>

#define NUM_ALERTS 11


class Loop;
class Reporter
{
private:
    std::ofstream output;

    const static std::string enumNames[NUM_ALERTS];
    bool toWarn[NUM_ALERTS];

public:
    enum AlertType {GENERIC, RANGE, UNINITIALISED_USE, UNDECLARED_USE, TYPE,
                    ZERODIVISION, USELESS_OP, BAD_STACK_USE, ARRAY_BOUNDS, COMPILER, DEADCODE, };

    Reporter(std::string filename);
    ~Reporter();
    void setWarning(AlertType type, bool toReport);
    void warn(AlertType type, const std::string& details, int linenum = -1);
    void error(AlertType type, const std::string& details, int linenum = -1);
    void optimising(AlertType type, const std::string& details, int linenum = -1);
    void info(const std::string& details, int linenum = -1);
    void addText(const std::string& text);
};


#endif
