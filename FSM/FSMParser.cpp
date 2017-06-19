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

string FSMParser::nextString()
{
    char c;
    infile.get(c);
    while (isspace(c) && infile.get(c));
    if (infile.eof()) throw runtime_error("command not finished");

    string ident;
    while (!isspace(c) && c != ';')
    {
        ident += c;
        infile.get(c);
        if (infile.eof()) throw runtime_error("identifier not finished");
    }
    infile.unget();

    return ident;
}

string FSMParser::nextCommand()
{
    char c = nextRealChar("End expected");

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

char FSMParser::nextRealChar(string error)
{
    char c;
    infile.get(c);
    while (isspace(c))
    {
        infile.get(c);
        if (infile.eof()) throw runtime_error(error);
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

    if (varN == "") throw runtime_error("Expected variable name");
    if (isReserved(varN)) throw runtime_error("'" + varN + "' is reserved");
    return varN;
}

set<string> FSMParser::resWords = {"end", "double", "string", "print", "jump", "jumpif", "push", "pop", "state"};
bool FSMParser::isReserved(const string& s)
{
    return (resWords.find(s) != resWords.end());
}

shared_ptr<Variable> FSMParser::getVar (string varN)
{
    unordered_map<string, shared_ptr<Variable>>::const_iterator it = variableMap.find(varN);
    if (it == variableMap.cend()) throw runtime_error("Unknown variable '" + varN + "'");
    return it->second;
}

FSM FSMParser::readFSM()
{
    FSM fsm;
    char c;

    //first scan - get variables and states
    string str = nextCommand();
    stateNameMap[str] = 0;
    int nextUnusedState = 1;
    while (!infile.eof() && str != "")
    {
        if (str == "double")
        {
            string varN = getVarName();
            unordered_map<string, shared_ptr<Variable>>::const_iterator it = variableMap.find(varN);
            if (it != variableMap.cend()) throw runtime_error("Variable '" + varN + "' declared multiple times");
            variableMap[varN] = shared_ptr<Variable>(new Variable(varN, 0.0));
            c = nextRealChar("Expected semicolon after variable declaration");
            if (c != ';') throw runtime_error("Expected semicolon after variable declaration");
        }
        else if (str == "string")
        {
            string varN = getVarName();
            unordered_map<string, shared_ptr<Variable>>::const_iterator it = variableMap.find(varN);
            if (it != variableMap.cend()) throw runtime_error("Variable '" + varN + "' declared multiple times");
            variableMap[varN] = shared_ptr<Variable>(new Variable(varN, ""));
            c = nextRealChar("Expected semicolon after variable declaration");
            if (c != ';') throw runtime_error("Expected semicolon after variable declaration");
        }
        else if (str == "end")
        {
            str = nextCommand();

            unordered_map<string, int>::const_iterator it = stateNameMap.find(str);
            if (it == stateNameMap.cend()) stateNameMap[str] = nextUnusedState++;
            else throw runtime_error("State '" + str + "' defined multiple times");
        }
        else while (infile.get(c) && c != ';' && c != '\n');
        str = nextCommand();
    }

    infile.clear();
    infile.seekg(0, ios::beg);

    while (!infile.eof())
    {
        //get state name
        while (infile.get(c) && isspace(c))
        if (infile.eof()) break;

        str = "";
        while (!isspace(c))
        {
            if (infile.eof()) throw "Reached EOF while parsing state name";
            str += c;
            infile.get(c);
        }

        int currentStateNum = checkState(str);

        shared_ptr<State> newState(new State(str, fsm));
        vector<shared_ptr<AbstractCommand>> commands;

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
                    shared_ptr<AbstractCommand> ref (new PrintCommand<shared_ptr<Variable>>(getVar(varN)));
                    commands.push_back(ref);
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
                            else
                            {
                                infile.unget();
                                c = '\\';
                            }
                        }
                        strToPrint += c;
                        infile.get(c);
                        if (infile.eof()) throw "Unexpected end when reading print string";
                    }

                    shared_ptr<AbstractCommand> ref (new PrintCommand<string>(strToPrint));
                    commands.push_back(ref);
                }
            }

            else if (str == "input")
            {
                string varN = nextString();
                shared_ptr<AbstractCommand> ref (new InputVarCommand(getVar(varN)));
                commands.push_back(ref);
            }

            else if (str == "jump")
            {
                string stateName = nextString();

                if (stateName == "pop") commands.push_back(shared_ptr<AbstractCommand>(new JumpTopCommand(fsm)));
                else
                {
                    int state = checkState(stateName);
                    shared_ptr<AbstractCommand> ref (new JumpCommand(state));
                    commands.push_back(ref);
                }
            }
            else if (str == "jumpif")
            {
                c = nextRealChar("Unfinished jumpif command");
                if (isdigit(c)) throw runtime_error("LHS of comparison must be a variable");

                //read LHS variable
                string varN = "";
                while (!infile.eof() && !isspace(c))
                {
                    varN += c;
                    infile.get(c);
                }

                shared_ptr<Variable> LHS = getVar(varN);

                c = nextRealChar("Unfinished jumpif command");
                //get operation type
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

                //get RHS
                string comparitor = nextString();

                string stateName = nextString();

                try
                {
                    double d = stod(comparitor);
                    if (stateName == "pop")
                    {
                        shared_ptr<JumpOnComparisonCommand<double>> ref(new JumpOnComparisonCommand<double>(LHS, d, fsm, opType));
                        commands.push_back(ref);
                    }
                    else
                    {
                        int state = checkState(stateName);
                        shared_ptr<JumpOnComparisonCommand<double>> ref(new JumpOnComparisonCommand<double>(LHS, d, state, opType));
                        commands.push_back(ref);
                    }

                }
                catch (invalid_argument e)
                {
                    if (stateName == "pop")
                    {
                        if (comparitor[0] == '"') //string
                        {
                            comparitor = peelQuotes(comparitor);
                            shared_ptr<JumpOnComparisonCommand<string>> ref(new JumpOnComparisonCommand<string>(LHS, comparitor, fsm, opType));
                            commands.push_back(ref);

                        }

                        else //identifier
                        {
                            shared_ptr<JumpOnComparisonCommand<shared_ptr<Variable>>> ref(
                                    new JumpOnComparisonCommand<shared_ptr<Variable>>(LHS, getVar(comparitor), fsm, opType));
                            commands.push_back(ref);
                        }
                    }
                    else
                    {
                        int state = checkState(stateName);
                        if (comparitor[0] == '"') //string
                        {
                            comparitor = peelQuotes(comparitor);
                            shared_ptr<JumpOnComparisonCommand<string>> ref(new JumpOnComparisonCommand<string>(LHS, comparitor, state, opType));
                            commands.push_back(ref);

                        }

                        else //identifier
                        {
                            shared_ptr<JumpOnComparisonCommand<shared_ptr<Variable>>> ref(
                                    new JumpOnComparisonCommand<shared_ptr<Variable>>(LHS, getVar(comparitor), state, opType));
                            commands.push_back(ref);
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
                    shared_ptr<PushCommand<double>> ref(new PushCommand<double>(point, fsm));
                    commands.push_back(ref);
                }

                else
                {
                    try
                    {
                        double d = stod(str);
                        shared_ptr<PushCommand<double>> ref(new PushCommand<double>(d, fsm));
                        commands.push_back(ref);
                    }
                    catch (invalid_argument e)
                    {
                        if (str[0] == '"') //string
                        {
                            str = peelQuotes(str);
                            shared_ptr<PushCommand<string>> ref(new PushCommand<string>(str, fsm));
                            commands.push_back(ref);

                        }

                        shared_ptr<PushCommand<shared_ptr<Variable>>> ref(new PushCommand<shared_ptr<Variable>>(getVar(str), fsm));
                        commands.push_back(ref);
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
                    shared_ptr<PopCommand> ref(new PopCommand(nullptr, fsm));
                    commands.push_back(ref);
                }
                else
                {
                    str = nextString();
                    shared_ptr<PopCommand> ref(new PopCommand(getVar(str), fsm));
                    commands.push_back(ref);
                }
            }

            else if (str == "string" || str == "double") getVarName();

            else //assigning to an identifier
            {
                std::shared_ptr<Variable> LHS = getVar(str);

                c = nextRealChar("Unfinished assignment command");
                if (c != '=') throw runtime_error("Expected assignment");
                string RHS = nextString();

                try
                {
                    double d = stod(RHS);
                    if (LHS->getType() != DOUBLE) throw runtime_error("Assigning double to " + LHS->getType());
                    shared_ptr<AssignVarCommand<double>> ref(new AssignVarCommand<double>(LHS, d)); //just a constant
                    commands.push_back(ref);
                }
                catch (invalid_argument e)
                {
                    if (RHS[0] == '"') //assigning string
                    {
                        if (LHS->getType() != STRING) throw runtime_error("Assigning string to " + LHS->getType());
                        RHS = peelQuotes(RHS);
                        shared_ptr<AssignVarCommand<string>> ref(new AssignVarCommand<string>(LHS, RHS));
                        commands.push_back(ref);
                    }

                    else
                    {
                        shared_ptr<Variable> RHSVar = getVar(RHS);

                        c = nextRealChar("Unfinished assignment command");
                        if (c == ';')
                        {
                            infile.unget();
                            shared_ptr<AssignVarCommand<shared_ptr<Variable>>> ref(new AssignVarCommand<shared_ptr<Variable>>(LHS, RHSVar));
                            commands.push_back(ref);
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
                                shared_ptr<EvaluateExprCommand<double>> ref (new EvaluateExprCommand<double>(LHS, RHSVar, d, expType));
                                commands.push_back(ref);
                            }
                            catch (invalid_argument e) //two vars
                            {
                                shared_ptr<EvaluateExprCommand<shared_ptr<Variable>>> ref (new EvaluateExprCommand<shared_ptr<Variable>>(LHS, RHSVar, getVar(term2), expType));
                                commands.push_back(ref);
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
        stateMap[currentStateNum] = newState;
    }

    vector<shared_ptr<State>> st;
    //works in order
    for (auto& p : stateMap) st.push_back(p.second);

    fsm.setStates(st);
    return fsm;
}
