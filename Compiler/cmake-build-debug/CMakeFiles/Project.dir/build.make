# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.7

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /home/abdul/Programs/clion-2016.3.3/bin/cmake/bin/cmake

# The command to remove a file.
RM = /home/abdul/Programs/clion-2016.3.3/bin/cmake/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/abdul/CLionProjects/Compiler

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/abdul/CLionProjects/Compiler/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/Project.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/Project.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/Project.dir/flags.make

CMakeFiles/Project.dir/main.cpp.o: CMakeFiles/Project.dir/flags.make
CMakeFiles/Project.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/abdul/CLionProjects/Compiler/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/Project.dir/main.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Project.dir/main.cpp.o -c /home/abdul/CLionProjects/Compiler/main.cpp

CMakeFiles/Project.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Project.dir/main.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/abdul/CLionProjects/Compiler/main.cpp > CMakeFiles/Project.dir/main.cpp.i

CMakeFiles/Project.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Project.dir/main.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/abdul/CLionProjects/Compiler/main.cpp -o CMakeFiles/Project.dir/main.cpp.s

CMakeFiles/Project.dir/main.cpp.o.requires:

.PHONY : CMakeFiles/Project.dir/main.cpp.o.requires

CMakeFiles/Project.dir/main.cpp.o.provides: CMakeFiles/Project.dir/main.cpp.o.requires
	$(MAKE) -f CMakeFiles/Project.dir/build.make CMakeFiles/Project.dir/main.cpp.o.provides.build
.PHONY : CMakeFiles/Project.dir/main.cpp.o.provides

CMakeFiles/Project.dir/main.cpp.o.provides.build: CMakeFiles/Project.dir/main.cpp.o


CMakeFiles/Project.dir/SymbolTable.cpp.o: CMakeFiles/Project.dir/flags.make
CMakeFiles/Project.dir/SymbolTable.cpp.o: ../SymbolTable.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/abdul/CLionProjects/Compiler/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/Project.dir/SymbolTable.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Project.dir/SymbolTable.cpp.o -c /home/abdul/CLionProjects/Compiler/SymbolTable.cpp

CMakeFiles/Project.dir/SymbolTable.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Project.dir/SymbolTable.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/abdul/CLionProjects/Compiler/SymbolTable.cpp > CMakeFiles/Project.dir/SymbolTable.cpp.i

CMakeFiles/Project.dir/SymbolTable.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Project.dir/SymbolTable.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/abdul/CLionProjects/Compiler/SymbolTable.cpp -o CMakeFiles/Project.dir/SymbolTable.cpp.s

CMakeFiles/Project.dir/SymbolTable.cpp.o.requires:

.PHONY : CMakeFiles/Project.dir/SymbolTable.cpp.o.requires

CMakeFiles/Project.dir/SymbolTable.cpp.o.provides: CMakeFiles/Project.dir/SymbolTable.cpp.o.requires
	$(MAKE) -f CMakeFiles/Project.dir/build.make CMakeFiles/Project.dir/SymbolTable.cpp.o.provides.build
.PHONY : CMakeFiles/Project.dir/SymbolTable.cpp.o.provides

CMakeFiles/Project.dir/SymbolTable.cpp.o.provides.build: CMakeFiles/Project.dir/SymbolTable.cpp.o


CMakeFiles/Project.dir/Lexer.cpp.o: CMakeFiles/Project.dir/flags.make
CMakeFiles/Project.dir/Lexer.cpp.o: ../Lexer.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/abdul/CLionProjects/Compiler/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/Project.dir/Lexer.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Project.dir/Lexer.cpp.o -c /home/abdul/CLionProjects/Compiler/Lexer.cpp

CMakeFiles/Project.dir/Lexer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Project.dir/Lexer.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/abdul/CLionProjects/Compiler/Lexer.cpp > CMakeFiles/Project.dir/Lexer.cpp.i

CMakeFiles/Project.dir/Lexer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Project.dir/Lexer.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/abdul/CLionProjects/Compiler/Lexer.cpp -o CMakeFiles/Project.dir/Lexer.cpp.s

CMakeFiles/Project.dir/Lexer.cpp.o.requires:

.PHONY : CMakeFiles/Project.dir/Lexer.cpp.o.requires

CMakeFiles/Project.dir/Lexer.cpp.o.provides: CMakeFiles/Project.dir/Lexer.cpp.o.requires
	$(MAKE) -f CMakeFiles/Project.dir/build.make CMakeFiles/Project.dir/Lexer.cpp.o.provides.build
.PHONY : CMakeFiles/Project.dir/Lexer.cpp.o.provides

CMakeFiles/Project.dir/Lexer.cpp.o.provides.build: CMakeFiles/Project.dir/Lexer.cpp.o


CMakeFiles/Project.dir/Token.cpp.o: CMakeFiles/Project.dir/flags.make
CMakeFiles/Project.dir/Token.cpp.o: ../Token.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/abdul/CLionProjects/Compiler/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/Project.dir/Token.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Project.dir/Token.cpp.o -c /home/abdul/CLionProjects/Compiler/Token.cpp

CMakeFiles/Project.dir/Token.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Project.dir/Token.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/abdul/CLionProjects/Compiler/Token.cpp > CMakeFiles/Project.dir/Token.cpp.i

CMakeFiles/Project.dir/Token.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Project.dir/Token.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/abdul/CLionProjects/Compiler/Token.cpp -o CMakeFiles/Project.dir/Token.cpp.s

CMakeFiles/Project.dir/Token.cpp.o.requires:

.PHONY : CMakeFiles/Project.dir/Token.cpp.o.requires

CMakeFiles/Project.dir/Token.cpp.o.provides: CMakeFiles/Project.dir/Token.cpp.o.requires
	$(MAKE) -f CMakeFiles/Project.dir/build.make CMakeFiles/Project.dir/Token.cpp.o.provides.build
.PHONY : CMakeFiles/Project.dir/Token.cpp.o.provides

CMakeFiles/Project.dir/Token.cpp.o.provides.build: CMakeFiles/Project.dir/Token.cpp.o


CMakeFiles/Project.dir/Parsing.cpp.o: CMakeFiles/Project.dir/flags.make
CMakeFiles/Project.dir/Parsing.cpp.o: ../Parsing.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/abdul/CLionProjects/Compiler/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/Project.dir/Parsing.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Project.dir/Parsing.cpp.o -c /home/abdul/CLionProjects/Compiler/Parsing.cpp

CMakeFiles/Project.dir/Parsing.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Project.dir/Parsing.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/abdul/CLionProjects/Compiler/Parsing.cpp > CMakeFiles/Project.dir/Parsing.cpp.i

CMakeFiles/Project.dir/Parsing.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Project.dir/Parsing.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/abdul/CLionProjects/Compiler/Parsing.cpp -o CMakeFiles/Project.dir/Parsing.cpp.s

CMakeFiles/Project.dir/Parsing.cpp.o.requires:

.PHONY : CMakeFiles/Project.dir/Parsing.cpp.o.requires

CMakeFiles/Project.dir/Parsing.cpp.o.provides: CMakeFiles/Project.dir/Parsing.cpp.o.requires
	$(MAKE) -f CMakeFiles/Project.dir/build.make CMakeFiles/Project.dir/Parsing.cpp.o.provides.build
.PHONY : CMakeFiles/Project.dir/Parsing.cpp.o.provides

CMakeFiles/Project.dir/Parsing.cpp.o.provides.build: CMakeFiles/Project.dir/Parsing.cpp.o


CMakeFiles/Project.dir/CodeGen.cpp.o: CMakeFiles/Project.dir/flags.make
CMakeFiles/Project.dir/CodeGen.cpp.o: ../CodeGen.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/abdul/CLionProjects/Compiler/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object CMakeFiles/Project.dir/CodeGen.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Project.dir/CodeGen.cpp.o -c /home/abdul/CLionProjects/Compiler/CodeGen.cpp

CMakeFiles/Project.dir/CodeGen.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Project.dir/CodeGen.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/abdul/CLionProjects/Compiler/CodeGen.cpp > CMakeFiles/Project.dir/CodeGen.cpp.i

CMakeFiles/Project.dir/CodeGen.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Project.dir/CodeGen.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/abdul/CLionProjects/Compiler/CodeGen.cpp -o CMakeFiles/Project.dir/CodeGen.cpp.s

CMakeFiles/Project.dir/CodeGen.cpp.o.requires:

.PHONY : CMakeFiles/Project.dir/CodeGen.cpp.o.requires

CMakeFiles/Project.dir/CodeGen.cpp.o.provides: CMakeFiles/Project.dir/CodeGen.cpp.o.requires
	$(MAKE) -f CMakeFiles/Project.dir/build.make CMakeFiles/Project.dir/CodeGen.cpp.o.provides.build
.PHONY : CMakeFiles/Project.dir/CodeGen.cpp.o.provides

CMakeFiles/Project.dir/CodeGen.cpp.o.provides.build: CMakeFiles/Project.dir/CodeGen.cpp.o


CMakeFiles/Project.dir/FunctionSymbol.cpp.o: CMakeFiles/Project.dir/flags.make
CMakeFiles/Project.dir/FunctionSymbol.cpp.o: ../FunctionSymbol.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/abdul/CLionProjects/Compiler/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object CMakeFiles/Project.dir/FunctionSymbol.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Project.dir/FunctionSymbol.cpp.o -c /home/abdul/CLionProjects/Compiler/FunctionSymbol.cpp

CMakeFiles/Project.dir/FunctionSymbol.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Project.dir/FunctionSymbol.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/abdul/CLionProjects/Compiler/FunctionSymbol.cpp > CMakeFiles/Project.dir/FunctionSymbol.cpp.i

CMakeFiles/Project.dir/FunctionSymbol.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Project.dir/FunctionSymbol.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/abdul/CLionProjects/Compiler/FunctionSymbol.cpp -o CMakeFiles/Project.dir/FunctionSymbol.cpp.s

CMakeFiles/Project.dir/FunctionSymbol.cpp.o.requires:

.PHONY : CMakeFiles/Project.dir/FunctionSymbol.cpp.o.requires

CMakeFiles/Project.dir/FunctionSymbol.cpp.o.provides: CMakeFiles/Project.dir/FunctionSymbol.cpp.o.requires
	$(MAKE) -f CMakeFiles/Project.dir/build.make CMakeFiles/Project.dir/FunctionSymbol.cpp.o.provides.build
.PHONY : CMakeFiles/Project.dir/FunctionSymbol.cpp.o.provides

CMakeFiles/Project.dir/FunctionSymbol.cpp.o.provides.build: CMakeFiles/Project.dir/FunctionSymbol.cpp.o


# Object files for target Project
Project_OBJECTS = \
"CMakeFiles/Project.dir/main.cpp.o" \
"CMakeFiles/Project.dir/SymbolTable.cpp.o" \
"CMakeFiles/Project.dir/Lexer.cpp.o" \
"CMakeFiles/Project.dir/Token.cpp.o" \
"CMakeFiles/Project.dir/Parsing.cpp.o" \
"CMakeFiles/Project.dir/CodeGen.cpp.o" \
"CMakeFiles/Project.dir/FunctionSymbol.cpp.o"

# External object files for target Project
Project_EXTERNAL_OBJECTS =

Project: CMakeFiles/Project.dir/main.cpp.o
Project: CMakeFiles/Project.dir/SymbolTable.cpp.o
Project: CMakeFiles/Project.dir/Lexer.cpp.o
Project: CMakeFiles/Project.dir/Token.cpp.o
Project: CMakeFiles/Project.dir/Parsing.cpp.o
Project: CMakeFiles/Project.dir/CodeGen.cpp.o
Project: CMakeFiles/Project.dir/FunctionSymbol.cpp.o
Project: CMakeFiles/Project.dir/build.make
Project: CMakeFiles/Project.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/abdul/CLionProjects/Compiler/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Linking CXX executable Project"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Project.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/Project.dir/build: Project

.PHONY : CMakeFiles/Project.dir/build

CMakeFiles/Project.dir/requires: CMakeFiles/Project.dir/main.cpp.o.requires
CMakeFiles/Project.dir/requires: CMakeFiles/Project.dir/SymbolTable.cpp.o.requires
CMakeFiles/Project.dir/requires: CMakeFiles/Project.dir/Lexer.cpp.o.requires
CMakeFiles/Project.dir/requires: CMakeFiles/Project.dir/Token.cpp.o.requires
CMakeFiles/Project.dir/requires: CMakeFiles/Project.dir/Parsing.cpp.o.requires
CMakeFiles/Project.dir/requires: CMakeFiles/Project.dir/CodeGen.cpp.o.requires
CMakeFiles/Project.dir/requires: CMakeFiles/Project.dir/FunctionSymbol.cpp.o.requires

.PHONY : CMakeFiles/Project.dir/requires

CMakeFiles/Project.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/Project.dir/cmake_clean.cmake
.PHONY : CMakeFiles/Project.dir/clean

CMakeFiles/Project.dir/depend:
	cd /home/abdul/CLionProjects/Compiler/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/abdul/CLionProjects/Compiler /home/abdul/CLionProjects/Compiler /home/abdul/CLionProjects/Compiler/cmake-build-debug /home/abdul/CLionProjects/Compiler/cmake-build-debug /home/abdul/CLionProjects/Compiler/cmake-build-debug/CMakeFiles/Project.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/Project.dir/depend

