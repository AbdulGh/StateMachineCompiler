#include "Reporter.h"

const std::string Reporter::enumNames[NUM_ALERTS] = {"", "OVERFLOW", "UNINITIALISED USE", "ZERO DIVISION", "USELESS OP", "COMPILER"};

Reporter::Reporter(std::string filename)
{
    output.open(filename);
    for (int i = 0; i < NUM_ALERTS; i++) toWarn[i] = true;
}

Reporter::~Reporter()
{
    output.close();
}

void Reporter::setWarning(AlertType type, bool toReport)
{
    toWarn[type] = toReport;
}

void Reporter::warn(AlertType type, std::string details)
{
    if (toWarn[type])
    {
        output << "WARNING : ";
        if (type != GENERIC) output << enumNames[type] << " : ";
        output << details << std::endl;
    }
}

void Reporter::error(AlertType type, std::string details)
{
    output << "ERROR : ";
    if (type != GENERIC) output << enumNames[type] << " : ";
    output << details << std::endl;
}