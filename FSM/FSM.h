#ifndef FSM_H
#define FSM_H

#include <vector>
#include <string>
#include <stack>
#include <map>
#include <set>
#include <fstream>

#include "Variable.h"
#include "State.h"
#include "Command.h"


class FSM
{
    friend class State;
    template<class T> friend class PushCommand;
    friend class PopCommand;
    friend class ReturnCommand;
    template<class T> friend class JumpOnComparisonCommand;

private:
    std::vector<std::unique_ptr<State>> states;
    std::stack<Variable::TaggedDataUnion> sharedStack;
    std::unordered_map<std::string, std::unique_ptr<Variable>> variableMap;

    class FSMParser
    {
    public:
        FSMParser(std::string filename, FSM& parsedFSM);
        void readFSM();

    private:
        std::unordered_map<std::string, int> stateNameMap;
        std::map<int, std::unique_ptr<State>> stateMap;
        std::ifstream infile;
        FSM& parsedFSM;

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


public:
    FSM(std::string& fileName);

    void run();
};


#endif
