#ifndef FSMPARSER_H
#define FSMPARSER_H

#include <fstream>
#include <map>
#include <memory>
#include <set>

#include "FSM.h"
#include "Variable.h"

class FSMParser
{
public:
    FSMParser(std::string filename);
    FSM readFSM();

private:
    std::unordered_map<std::string, int> stateNameMap;
    std::map<int, std::unique_ptr<State>> stateMap;
    std::unordered_map<std::string, std::unique_ptr<Variable>> variableMap;
    std::ifstream infile;

    static std::set<std::string> resWords;
    bool isReserved(const std::string&);

    Variable* getVar(std::string varN);

    std::string nextString();
    std::string nextCommand(bool expecting = true);
    std::string getVarName();
    std::string peelQuotes(std::string);
    ComparisonOp readComparisonOp();
    int checkState(std::string);
    char nextRealChar(std::string);
};


#endif