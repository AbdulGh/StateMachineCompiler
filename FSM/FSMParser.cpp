#include "FSMParser.h"
#include "Command.h"

using namespace std;

FSMParser::FSMParser(string filename)
{
    infile.open(filename);
    if (!infile) throw runtime_error("Could not open file '" + filename + "'");
}

string FSMParser::peelQuotes(string in)
{
    if (in[0] != '"' || in[in.length() - 1] != '"') throw runtime_error("Badly quoted string");
    in.erase(0,1); in.erase(in.length()-1, 1);
    return in;
}

int FSMParser::checkState(string str)
{
    unordered_map<string, int>::const_iterator it = stateNameMap.find(str);
    if (it == stateNameMap.cend()) throw runtime_error("State '" + str + "' not defined");
    return it->second;
}

ComparisonOp FSMParser::readComparisonOp()
{
    char c = nextRealChar("Unfinished jumpif command");
    ComparisonOp opType;
    switch(c)
    {
        case '<':
            infile.get(c);
            if (c == '=') opType = ComparisonOp::LE;
            else
            {
                infile.unget();
                opType = ComparisonOp::LT;
            }
            break;

        case '>':
            infile.get(c);
            if (c == '=') opType = ComparisonOp::GE;
            else
            {
                infile.unget();
                opType = ComparisonOp::GT;
            }
            break;

        case '=':
            opType = ComparisonOp::EQ;
            break;

        case '!':
            infile.get(c);
            if (c == '=')
            {
                opType = ComparisonOp::NEQ;
                break;
            }

        default:
            throw runtime_error("Strange comparison detected");
    }
    return opType;
}

string FSMParser::nextString()
{
    char c;
    infile.get(c);
    while (isspace(c) && infile.get(c));
    if (infile.eof()) throw runtime_error("command not finished");

    string ident;
    if (c == '"')
    {
        ident += c;
        while (infile.get(c))
        {
            ident += c;
            if (c == '"') break;
            else if (infile.eof()) throw runtime_error("identifier not finished");
        }
    }

    else
    {
        while (!isspace(c) && c != ';')
        {
            ident += c;
            infile.get(c);
            if (infile.eof()) throw runtime_error("identifier not finished");
        }
        infile.unget();
    }

    return ident;
}

string FSMParser::nextCommand(bool expecting)
{
    char c = nextRealChar(expecting ? "End expected" : "");

    string str;
    while (!isspace(c) && !infile.eof())
    {
        if (c == ';') break;
        str += c;
        infile.get(c);
    }
    if (!infile.eof()) infile.unget();
    return str;
}

char FSMParser::nextRealChar(string error = "")
{
    char c;
    infile.get(c);
    while (isspace(c))
    {
        infile.get(c);
        if (infile.eof())
        {
            if (error != "") throw runtime_error(error);
            else return -1;
        }
    }
    return c;
}

string FSMParser::getVarName()
{
    string varN = "";
    char c = nextRealChar("Unexpected end when reading variable");
    if (isdigit(c)) throw runtime_error("Variables cannot begin with a digit");
    while (c != ';' && !isspace(c))
    {
        varN += c;
        infile.get(c);
    }
    infile.unget();

    if (varN.empty()) throw runtime_error("Expected variable name");
    if (isReserved(varN)) throw runtime_error("'" + varN + "' is reserved");
    return varN;
}

set<string> FSMParser::resWords = {"end", "double", "string", "print", "jump", "jumpif", "push", "pop", "state", "return"};
bool FSMParser::isReserved(const string& s)
{
    return (resWords.find(s) != resWords.end());
}

Variable* FSMParser::getVar(string varN)
{
    unordered_map<string, unique_ptr<Variable>>::const_iterator it = variableMap.find(varN);
    if (it == variableMap.cend()) throw runtime_error("Unknown variable '" + varN + "'");
    return it->second.get();
}

FSM FSMParser::readFSM()
{
    FSM fsm;
    char c;

    //first scan - get variables and states
    string str = nextCommand();
    stateNameMap[str] = 0;
    int nextUnusedState = 1;
    while (!infile.eof() && !str.empty())
    {
        if (str == "double")
        {
            string varN = getVarName();
            variableMap[varN] = make_unique<Variable>(varN, 0.0);
            c = nextRealChar("Expected semicolon after variable declaration");
            if (c != ';') throw runtime_error("Expected semicolon after variable declaration");
        }
        else if (str == "string")
        {
            string varN = getVarName();
            variableMap[varN] = make_unique<Variable>(varN, "");
            c = nextRealChar("Expected semicolon after variable declaration");
            if (c != ';') throw runtime_error("Expected semicolon after variable declaration");
        }
        else if (str == "end")
        {
            if (infile >> str && !str.empty())
            {
                unordered_map<string, int>::const_iterator it = stateNameMap.find(str);
                if (it == stateNameMap.cend()) stateNameMap[str] = nextUnusedState++;
                else throw runtime_error("State '" + str + "' defined multiple times");
            }
            else break;
        }
        else while (infile.get(c) && c != ';' && c != '\n');
        str = nextCommand();
    }

    infile.clear();
    infile.seekg(0, ios::beg);

    while (!infile.eof())
    {
        //get state name
        while (infile.get(c) && isspace(c));
        if (infile.eof()) break;

        str = "";
        while (!isspace(c))
        {
            if (infile.eof()) throw "Reached EOF while parsing state name";
            str += c;
            infile.get(c);
        }

        int currentStateNum = checkState(str);

        unique_ptr<State> newState(new State(str, fsm));
        vector<unique_ptr<AbstractCommand>> commands;

        str = nextCommand();
        while(str != "end")
        {
            if (infile.eof()) throw runtime_error("Unexpected end while parsing state '" + newState->getName() + "'");

            if (str == "print")
            {
                infile.get(c);

                while (isspace(c))
                {
                    infile.get(c);
                    if (infile.eof()) throw "Unexpected end during print command";
                }

                if (c != '"')
                {
                    string varN;
                    while (!isspace(c))
                    {
                        if (c == ';')
                        {
                            infile.unget();
                            break;
                        }
                        varN += c;
                        infile.get(c);
                        if (infile.eof()) throw runtime_error("identifier not finished");
                    }
                    commands.push_back(make_unique<PrintCommand<Variable*>>(getVar(varN)));
                }

                else
                {
                    infile.get(c);
                    string strToPrint;
                    while (c != '"')
                    {
                        if (c == '\\')
                        {
                            infile.get(c);
                            if (c == 'n') c = '\n';
                            else if (c == '\\') c = '\\';
                        }
                        strToPrint += c;
                        infile.get(c);
                        if (infile.eof()) throw "Unexpected end when reading print string";
                    }
                    commands.push_back(make_unique<PrintCommand<string>>(strToPrint));
                }
            }

            else if (str == "input")
            {
                string varN = nextString();
                commands.push_back(make_unique<InputVarCommand>(getVar(varN)));
            }

            else if (str == "return")
            {
                commands.push_back(make_unique<ReturnCommand>(fsm));
            }

            else if (str == "jump")
            {
                string stateName = nextString();
                if (stateName == "pop") throw "depreciated";
                int state = checkState(stateName);
                commands.push_back(make_unique<JumpCommand>(ref));
            }

            else if (str == "jumpif")
            {
                c = nextRealChar("Unfinished jumpif command");
                if (isdigit(c)) ///LHS is double literal
                {
                    auto readNumber = [&, this](char first) -> double
                    {
                        double LHS;
                        string readString;
                        while (!isspace(first))
                        {
                            readString += first;
                            infile.get(first);
                            if (!infile) throw "bad number";
                        }

                        try
                        {
                            LHS = stod(readString);
                        }
                        catch(invalid_argument&)
                        {
                            throw "bad number";
                        }
                        return LHS;
                    };
                    double LHS = readNumber(c);

                    ComparisonOp op = readComparisonOp();

                    //read RHS variable
                    c = nextRealChar("unexpected end during jumpif");

                    if (isdigit(c)) //both numbers
                    {
                        double RHS = readNumber(c);
                        if (evaluateComparisonOp<double>(LHS, op, RHS))
                        {

                        }
                    }

                        string varN;
                    while (!infile.eof() && !isspace(c))
                    {
                        varN += c;
                        infile.get(c);
                    }
                }

                //read LHS variable
                string varN;
                while (!infile.eof() && !isspace(c))
                {
                    varN += c;
                    infile.get(c);
                }

                Variable* LHS = getVar(varN);

                ComparisonOp opType = readComparisonOp();

                //get RHS
                string comparitor = nextString();

                string stateName = nextString();

                try
                {
                    double d = stod(comparitor);
                    if (stateName == "pop")
                    {
                        commands.push_back(make_unique<JumpOnComparisonCommand<double>>(LHS, d, fsm, opType));
                    }
                    else
                    {
                        int state = checkState(stateName);
                        commands.push_back(make_unique<JumpOnComparisonCommand<double>>(LHS, d, state, opType));
                    }

                }
                catch (invalid_argument&)
                {
                    if (stateName == "pop")
                    {
                        if (comparitor[0] == '"') //string
                        {
                            comparitor = peelQuotes(comparitor);
                            commands.push_back(make_unique<JumpOnComparisonCommand<string>>(LHS, comparitor, fsm, opType));

                        }

                        else //identifier
                        {
                            commands.push_back(make_unique<JumpOnComparisonCommand<Variable*>>
                                                       (LHS, getVar(comparitor), fsm, opType));
                        }
                    }
                    else
                    {
                        int state = checkState(stateName);
                        if (comparitor[0] == '"') //string
                        {
                            comparitor = peelQuotes(comparitor);
                            commands.push_back(make_unique<JumpOnComparisonCommand<string>>(LHS, comparitor, state, opType));

                        }

                        else //identifier
                        {
                            commands.push_back(make_unique<JumpOnComparisonCommand<Variable*>(LHS, getVar(comparitor),
                                                                                              state, opType);
                        }
                    }
                }
            }

            else if (str == "push")
            {
                str = nextString();

                if (str == "state")
                {
                    str = nextString();
                    int point = checkState(str);
                    commands.push_back(make_unique<PushCommand<double>>(point, fsm));
                }

                else
                {
                    try
                    {
                        double d = stod(str);
                        commands.push_back(make_unique<PushCommand<double>>(d, fsm));
                    }
                    catch (invalid_argument e)
                    {
                        if (str[0] == '"') //string
                        {
                            str = peelQuotes(str);
                            commands.push_back(make_unique<PushCommand<string>>(str, fsm));
                        }
                        else commands.push_back(make_unique<PushCommand<Variable*>>(getVar(str), fsm));
                    }
                }
            }

            else if (str == "pop")
            {
                c = nextRealChar("Unexpected end when reading pop command");
                infile.unget();
                if (c == ';')
                {
                    str = nextString();
                    commands.push_back(make_unique<PopCommand>(nullptr, fsm));
                }
                else
                {
                    str = nextString();
                    commands.push_back(make_unique<PopCommand>(getVar(str), fsm));
                }
            }

            else if (str == "string" || str == "double") getVarName();

            else //assigning to an identifier
            {
                Variable* LHS = getVar(str);

                c = nextRealChar("Unfinished assignment command");
                if (c != '=') throw runtime_error("Expected assignment");
                string RHS = nextString();

                try
                {
                    double d = stod(RHS);
                    if (LHS->getType() != DOUBLE) throw runtime_error("Assigning double to " + LHS->getType());
                    commands.push_back(make_unique<AssignVarCommand<double>>(LHS, d)); //just a constant
                }
                catch (invalid_argument e)
                {
                    if (RHS[0] == '"') //assigning string
                    {
                        if (LHS->getType() != STRING) throw runtime_error("Assigning string to " + LHS->getType());
                        RHS = peelQuotes(RHS);
                        commands.push_back(make_unique<AssignVarCommand<string>>(LHS, RHS));
                    }

                    else
                    {
                        Variable* RHSVar = getVar(RHS);

                        c = nextRealChar("Unfinished assignment command");
                        if (c == ';')
                        {
                            infile.unget();
                            commands.push_back(make_unique<AssignVarCommand<Variable*>>(LHS, RHSVar));
                        }

                        else //some expression
                        {
                            ExpressionType expType;

                            switch(c)
                            {
                                case '+':
                                    expType = PLUS;
                                    break;

                                case '-':
                                    expType = MINUS;
                                    break;

                                case '/':
                                    expType = DIV;
                                    break;

                                case '*':
                                    expType = MUL;
                                    break;

                                case '%':
                                    expType = MOD;
                                    break;

                                case '^':
                                    expType = POW;
                                    break;

                                case '&':
                                    expType = AND;
                                    break;

                                case '|':
                                    expType = OR;
                                    break;

                                default:
                                    throw runtime_error("Strange expression type detected");
                            }

                            string term2 = nextString();
                            try
                            {
                                double d = stod(term2);
                                commands.push_back(make_unique<EvaluateExprCommand<double>>(LHS, RHSVar, d, expType));
                            }
                            catch (invalid_argument&) //two vars
                            {
                                commands.push_back(make_unique<EvaluateExprCommand<Variable*>>
                                                           (LHS, RHSVar, getVar(term2), expType));
                            }
                        }
                    }
                }
            }

            infile.get(c);
            while (isspace(c))
            {
                if (infile.eof()) throw runtime_error("State '" + newState->getName() + "' not ended");
                infile.get(c);
            }
            if (c != ';') throw "Expected semicolon";

            str = nextCommand();
        }

        newState->setInstructions(commands);
        stateMap[currentStateNum] = move(newState);
    }

    vector<unique_ptr<State>> st;
    //works in order
    for (auto& p : stateMap) st.push_back(p.second);

    fsm.setStates(st);
    return fsm;
}
