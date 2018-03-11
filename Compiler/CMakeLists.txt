cmake_minimum_required(VERSION 3.6)
project(Project)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-fPIC")

set(SOURCE_FILES source/main.cpp source/compile/SymbolTable.cpp source/compile/SymbolTable.h source/compile/Lexer.cpp source/compile/Lexer.h source/compile/Token.cpp source/compile/Lexer.cpp source/compile/Lexer.h source/compile/Parsing.cpp
        source/compile/Compiler.h source/compile/CodeGen.cpp source/compile/Functions.cpp source/compile/Functions.h source/compile/ExpressionCodeGenerator.cpp source/compile/ExpressionCodeGenerator.h
        source/compile/ExpressionTreeNodes.cpp source/Command.h source/CFGOpt/Optimiser.cpp source/CFGOpt/Optimiser.h source/CFGOpt/CFG.cpp source/CFGOpt/CFG.h source/symbolic/SymbolicVariable.cpp source/symbolic/SymbolicVariables.h
        source/symbolic/SymbolicVarSet.cpp source/symbolic/SymbolicVarSet.h source/symbolic/SymbolicExecution.cpp source/symbolic/SymbolicExecution.h source/compile/Reporter.cpp source/compile/Reporter.h source/symbolic/SymbolicStack.cpp
        source/symbolic/SymbolicStack.h source/symbolic/CommandAcceptSymbolicExecution.cpp source/compile/Compiler.cpp source/symbolic/SymbolicDouble.cpp source/symbolic/SymbolicString.cpp source/CFGOpt/CFGNodes.cpp source/compile/FunctionTable.cpp
        source/CFGOpt/DataFlow.cpp source/CFGOpt/DataFlow.h source/CFGOpt/LengTarj.cpp source/CFGOpt/LengTarj.h source/CFGOpt/Loop.h source/symbolic/LoopValidation.cpp source/symbolic/SymbolicArray.h source/symbolic/SymbolicVarWrappers.h source/symbolic/CommandFunctionality.cpp)
add_executable(Project ${SOURCE_FILES})

include (CTest)
find_program(MEMORYCHECK_COMMAND valgrind)
set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --track-origins=yes")
add_test(compilertest Project)