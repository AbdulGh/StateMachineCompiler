cmake_minimum_required(VERSION 3.6)
project(Project)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp SymbolTable.cpp SymbolTable.h Lexer.cpp Lexer.h Token.cpp Lexer.cpp Lexer.h Parsing.cpp
    Compiler.h CodeGen.cpp FunctionCodeGen.cpp FunctionCodeGen.h ExpressionCodeGenerator.cpp ExpressionCodeGenerator.h
    ExpressionTreeNodes.cpp Command.h State.cpp State.h Optimiser.cpp Optimiser.h Enums.h)
add_executable(Project ${SOURCE_FILES})

include (CTest)
find_program(MEMORYCHECK_COMMAND valgrind)
set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --track-origins=yes")
add_test(compilertest Project)