cmake_minimum_required(VERSION 3.6)
project(Project)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp SymbolTable.cpp SymbolTable.h Lexer.cpp Lexer.h Token.cpp Lexer.cpp Lexer.h)
add_executable(Project ${SOURCE_FILES})