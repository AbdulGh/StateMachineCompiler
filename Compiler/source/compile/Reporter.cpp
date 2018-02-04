#include "Reporter.h"
#include "../CFGOpt/Loop.h"

const std::string Reporter::enumNames[NUM_ALERTS] =
        {"", "OVERFLOW", "UNINITIALISED USE", "UNDECLARED USE",
         "TYPE", "ZERO DIVISION", "USELESS OP", "STACK USE", "ARRAY BOUNDS", "COMPILER", "DEAD CODE"};

Reporter::Reporter(std::string filename)
{
    output.open(filename);
    for (int i = 0; i < NUM_ALERTS; ++i) toWarn[i] = true;
}

Reporter::~Reporter()
{
    output.close();
}

void Reporter::setWarning(AlertType type, bool toReport)
{
    toWarn[type] = toReport;
}

void Reporter::warn(AlertType type, const std::string& details, int linenum)
{
    if (toWarn[type])
    {
        output << "WARNING : ";
        if (type != GENERIC) output << enumNames[type] << " : ";
        if (linenum != -1) output << " (line " << linenum << ")";
        output << details << '\n';
    }
}

void Reporter::error(AlertType type, const std::string& details, int linenum)
{
    output << "ERROR : ";
    if (type != GENERIC) output << enumNames[type] << " : ";
    output << details;
    if (linenum != -1) output << " (line " << linenum << ")";
    output << '\n';
}

void Reporter::optimising(AlertType type, const std::string& details, int linenum)
{
    output << "OPTIMISATION : ";
    if (type != GENERIC) output << enumNames[type] << " : ";
    output << details;
    if (linenum != -1) output << " (line " << linenum << ")";
    output << '\n';
}

void Reporter::info(const std::string& details, int linenum)
{
    output << details;
    if (linenum != -1) output << " (line " << linenum << ")";
    output << '\n';
}

void Reporter::addText(const std::string& text)
{
    output << text;
}