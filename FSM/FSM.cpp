#include <iostream>
#include <fstream>
#include <unordered_map>
#include <memory>
#include "FSM.h"

using namespace std;

FSM::FSM(std::string filename)
{
    ifstream infile(filename);

    if (!infile) throw runtime_error("Could not open file '" + filename + "'");

    unordered_map<string, int> stateMap;
    int nextUnusedState = 0;
    auto checkState = [&stateMap, &nextUnusedState] (std::string str) -> int
    {
        unordered_map<string,int>::const_iterator pair = stateMap.find(str);
        if (pair == stateMap.end())
        {
            stateMap[str] = nextUnusedState;
            return nextUnusedState++;
        }
        return pair->second;
    };

    auto getUntilSemic = [] (std::ifstream infile) -> string
    {
        char c;
        infile.get(c);
        while (isspace(c))
        {
            infile.get(c);
            if (infile.eof()) throw runtime_error("command not finished.");
        }

        string ident;

        while (!isspace(c))
        {
            if (c == ';')
            {
                infile.unget();
                break;
            }
            ident += c;
            infile.get(c);
            if (infile.eof()) throw runtime_error("identifier not finished.");
        }

        return ident;
    };

    while (!infile.eof())
    {
        char c;
        infile.get(c);

        while (isspace(c))
        {
            if (infile.eof()) throw "File is empty.";
            infile.get(c);
        }

        //get state name
        string str = "";

        while (!isspace(c))
        {
            if (infile.eof()) throw "Reached EOF while parsing state name.";
            str += c;
            infile.get(c);
        }

        int currentStateNum = checkState(str);

        shared_ptr<State> newState(new State(str));
        vector<shared_ptr<AbstractCommand>> commands;

        infile >> str;
        while(str != "end")
        {
            if (infile.eof()) throw runtime_error("Unexpected end while parsing state '" + newState->getName() + "'");

            if (str == "print")
            {
                infile.get(c);

                while (isspace(c))
                {
                    infile.get(c);
                    if (infile.eof()) throw "Unexpected end when finding string to print.";
                }

                if (c != '"') throw "Expected quote after print operation.";

                infile.get(c);
                string strToPrint;
                while (c != '"')
                {
                    strToPrint += c;
                    infile.get(c);
                    if (infile.eof()) throw "Unexpected end when reading print string.";
                }

                shared_ptr<AbstractCommand> ref (new PrintConstCommand(strToPrint));
                commands.push_back(ref);
            }
            else if (str == "goto")
            {
                string stateName = getUntilSemic(infile);

                int state = checkState(stateName);

                shared_ptr<AbstractCommand> ref (new JumpCommand(state));
                commands.push_back(ref);
            }
            else if (str == "string")
            {
                string ident = getUntilSemic(infile);
                shared_ptr<AbstractCommand> ref (new DeclareVarCommand(Variable::Type::STRING, ident));
                commands.push_back(ref);
            }
            else if (str == "int")
            {
                string ident = getUntilSemic(infile);
                shared_ptr<AbstractCommand> ref (new DeclareVarCommand(Variable::Type::INT, ident));
                commands.push_back(ref);
            }
            else if (str == "double")
            {
                string ident = getUntilSemic(infile);
                shared_ptr<AbstractCommand> ref (new DeclareVarCommand(Variable::Type::DOUBLE, ident));
                commands.push_back(ref);
            }

            infile.get(c);
            while (isspace(c))
            {
                if (infile.eof()) throw runtime_error("State '" + newState->getName() + "' not ended.");
                infile.get(c);
            }
            if (c != ';') throw "Expected semicolon.";

            infile >> str;
        }

        newState->setInstructions(commands);
        states.push_back(newState);

    }
}

void FSM::run()
{
    int currentStateNum = 0;
    ScopeManager sm;
    while (currentStateNum != -1)
    {
        auto& currentState = states.at(currentStateNum);
        currentState->run(sm);
        currentStateNum = currentState->nextState();
    }
}