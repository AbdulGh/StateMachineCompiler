#include <iostream>
#include <limits>
#include <cstring>

#include "compile/Compiler.h"
#include "compile/Lexer.h"
#include "symbolic/SymbolicArray.h"
#include "CFGOpt/Optimiser.h"

using namespace std;

void doHelp()
{
    cout << "Required parameters:\n";
    cout << "-f : Source code filename\n";
    cout << "Optional parameters:\n";
    cout << "-w : Warning output\n";
    cout << "-o : Produced code output\n";
    cout << "-g b/a: CFG DOT output before/after symbolic execution\n";
    cout << "-nv : Don't attempt verification\n";
    cout << "-no : Don't perform dataflow/state collapsing\n";
    cout << "-nd : Don't remove unreachable code\n";
}

int main(int argc, char* argv[])
{
    /*Lexer lexer2;
    vector<Token> st2 = lexer2.tokenize("/home/abdul/CLionProjects/Compiler/examples/report examples/array.f");
    ofstream warnings("../warnings.txt");
    Reporter r2(warnings.rdbuf());
    Compiler c2(st2, r2);
    stringstream pleaseWork2;
    c2.compile(true, true, true, "", true, "");
    cout << pleaseWork2.str();
    return 0;*/

    if (argc == 1 || argv[1] == "-h")
    {
        doHelp();
        return 0;
    }

    string inputfile = "";
    string warningfile = "";
    string outputfile = "";
    string graphfile = "";
    bool graphbefore = false;
    bool verify = true;
    bool opt = true;
    bool deadcode = true;

    int counter = 1;

    while (counter < argc)
    {
        if (strcmp(argv[counter], "-f") == 0)
        {
            ++counter;
            if (counter == argc) throw runtime_error("Expected filename after -f (-h for help)");
            inputfile = argv[counter];
        }
        else if (strcmp(argv[counter], "-w") == 0)
        {
            ++counter;
            if (counter == argc) throw runtime_error("Expected filename after -w (-h for help)");
            warningfile = argv[counter];
        }
        else if (strcmp(argv[counter], "-o") == 0)
        {
            ++counter;
            if (counter == argc) throw runtime_error("Expected filename after -o (-h for help)");
            outputfile = argv[counter];
        }
        else if (strcmp(argv[counter], "-g") == 0)
        {
            if (counter >= argc - 2) throw runtime_error("Not enough parrameters given to -g (-h for help)");
            ++counter;
            graphbefore = strcmp(argv[counter], "b") == 0;
            ++counter;
            graphfile = argv[counter];
        }
        else if (strcmp(argv[counter], "-no") == 0) opt = false;
        else if (strcmp(argv[counter], "-nv") == 0) verify = false;
        else if (strcmp(argv[counter], "-nd") == 0) deadcode = false;
        ++counter;
    }

    if (inputfile.empty()) throw runtime_error("No input file (-h for help)");

    ofstream fout;
    if (!warningfile.empty())
    {
        fout.open(warningfile);
        if (!fout.good()) throw runtime_error("Could not open filename '" + warningfile + "' for warning output.");
    }
    else fout.setstate(std::ios::badbit); //disables output

    Reporter r(fout.rdbuf());

    Lexer lexer;
    vector<Token> st = lexer.tokenize(inputfile);
    Compiler c(st, r);
    c.compile(opt, deadcode, verify, graphfile, graphbefore, outputfile);

    fout.close();
    return 0;
}