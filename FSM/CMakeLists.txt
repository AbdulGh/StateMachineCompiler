cmake_minimum_required(VERSION 3.6)
project(FSM)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp Command.cpp Command.h State.cpp State.h Variable.h FSM.cpp FSM.h FSMParser.cpp FSMParser.h Enums.h Variable.cpp)
add_executable(FSM ${SOURCE_FILES})