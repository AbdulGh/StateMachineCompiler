cmake_minimum_required(VERSION 3.6)
project(FSM)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp Command.cpp Command.h State.cpp State.h ScopeManager.cpp ScopeManager.h FSM.cpp FSM.h)
add_executable(FSM ${SOURCE_FILES})