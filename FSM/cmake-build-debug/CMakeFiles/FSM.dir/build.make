# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.6

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
CMAKE_SOURCE_DIR = /home/abdul/CLionProjects/FSM

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/abdul/CLionProjects/FSM/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/FSM.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/FSM.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/FSM.dir/flags.make

CMakeFiles/FSM.dir/main.cpp.o: CMakeFiles/FSM.dir/flags.make
CMakeFiles/FSM.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/abdul/CLionProjects/FSM/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/FSM.dir/main.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/FSM.dir/main.cpp.o -c /home/abdul/CLionProjects/FSM/main.cpp

CMakeFiles/FSM.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/FSM.dir/main.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/abdul/CLionProjects/FSM/main.cpp > CMakeFiles/FSM.dir/main.cpp.i

CMakeFiles/FSM.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/FSM.dir/main.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/abdul/CLionProjects/FSM/main.cpp -o CMakeFiles/FSM.dir/main.cpp.s

CMakeFiles/FSM.dir/main.cpp.o.requires:

.PHONY : CMakeFiles/FSM.dir/main.cpp.o.requires

CMakeFiles/FSM.dir/main.cpp.o.provides: CMakeFiles/FSM.dir/main.cpp.o.requires
	$(MAKE) -f CMakeFiles/FSM.dir/build.make CMakeFiles/FSM.dir/main.cpp.o.provides.build
.PHONY : CMakeFiles/FSM.dir/main.cpp.o.provides

CMakeFiles/FSM.dir/main.cpp.o.provides.build: CMakeFiles/FSM.dir/main.cpp.o


CMakeFiles/FSM.dir/Command.cpp.o: CMakeFiles/FSM.dir/flags.make
CMakeFiles/FSM.dir/Command.cpp.o: ../Command.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/abdul/CLionProjects/FSM/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/FSM.dir/Command.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/FSM.dir/Command.cpp.o -c /home/abdul/CLionProjects/FSM/Command.cpp

CMakeFiles/FSM.dir/Command.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/FSM.dir/Command.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/abdul/CLionProjects/FSM/Command.cpp > CMakeFiles/FSM.dir/Command.cpp.i

CMakeFiles/FSM.dir/Command.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/FSM.dir/Command.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/abdul/CLionProjects/FSM/Command.cpp -o CMakeFiles/FSM.dir/Command.cpp.s

CMakeFiles/FSM.dir/Command.cpp.o.requires:

.PHONY : CMakeFiles/FSM.dir/Command.cpp.o.requires

CMakeFiles/FSM.dir/Command.cpp.o.provides: CMakeFiles/FSM.dir/Command.cpp.o.requires
	$(MAKE) -f CMakeFiles/FSM.dir/build.make CMakeFiles/FSM.dir/Command.cpp.o.provides.build
.PHONY : CMakeFiles/FSM.dir/Command.cpp.o.provides

CMakeFiles/FSM.dir/Command.cpp.o.provides.build: CMakeFiles/FSM.dir/Command.cpp.o


CMakeFiles/FSM.dir/State.cpp.o: CMakeFiles/FSM.dir/flags.make
CMakeFiles/FSM.dir/State.cpp.o: ../State.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/abdul/CLionProjects/FSM/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/FSM.dir/State.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/FSM.dir/State.cpp.o -c /home/abdul/CLionProjects/FSM/State.cpp

CMakeFiles/FSM.dir/State.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/FSM.dir/State.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/abdul/CLionProjects/FSM/State.cpp > CMakeFiles/FSM.dir/State.cpp.i

CMakeFiles/FSM.dir/State.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/FSM.dir/State.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/abdul/CLionProjects/FSM/State.cpp -o CMakeFiles/FSM.dir/State.cpp.s

CMakeFiles/FSM.dir/State.cpp.o.requires:

.PHONY : CMakeFiles/FSM.dir/State.cpp.o.requires

CMakeFiles/FSM.dir/State.cpp.o.provides: CMakeFiles/FSM.dir/State.cpp.o.requires
	$(MAKE) -f CMakeFiles/FSM.dir/build.make CMakeFiles/FSM.dir/State.cpp.o.provides.build
.PHONY : CMakeFiles/FSM.dir/State.cpp.o.provides

CMakeFiles/FSM.dir/State.cpp.o.provides.build: CMakeFiles/FSM.dir/State.cpp.o


CMakeFiles/FSM.dir/ScopeManager.cpp.o: CMakeFiles/FSM.dir/flags.make
CMakeFiles/FSM.dir/ScopeManager.cpp.o: ../ScopeManager.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/abdul/CLionProjects/FSM/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/FSM.dir/ScopeManager.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/FSM.dir/ScopeManager.cpp.o -c /home/abdul/CLionProjects/FSM/ScopeManager.cpp

CMakeFiles/FSM.dir/ScopeManager.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/FSM.dir/ScopeManager.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/abdul/CLionProjects/FSM/ScopeManager.cpp > CMakeFiles/FSM.dir/ScopeManager.cpp.i

CMakeFiles/FSM.dir/ScopeManager.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/FSM.dir/ScopeManager.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/abdul/CLionProjects/FSM/ScopeManager.cpp -o CMakeFiles/FSM.dir/ScopeManager.cpp.s

CMakeFiles/FSM.dir/ScopeManager.cpp.o.requires:

.PHONY : CMakeFiles/FSM.dir/ScopeManager.cpp.o.requires

CMakeFiles/FSM.dir/ScopeManager.cpp.o.provides: CMakeFiles/FSM.dir/ScopeManager.cpp.o.requires
	$(MAKE) -f CMakeFiles/FSM.dir/build.make CMakeFiles/FSM.dir/ScopeManager.cpp.o.provides.build
.PHONY : CMakeFiles/FSM.dir/ScopeManager.cpp.o.provides

CMakeFiles/FSM.dir/ScopeManager.cpp.o.provides.build: CMakeFiles/FSM.dir/ScopeManager.cpp.o


CMakeFiles/FSM.dir/FSM.cpp.o: CMakeFiles/FSM.dir/flags.make
CMakeFiles/FSM.dir/FSM.cpp.o: ../FSM.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/abdul/CLionProjects/FSM/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/FSM.dir/FSM.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/FSM.dir/FSM.cpp.o -c /home/abdul/CLionProjects/FSM/FSM.cpp

CMakeFiles/FSM.dir/FSM.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/FSM.dir/FSM.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/abdul/CLionProjects/FSM/FSM.cpp > CMakeFiles/FSM.dir/FSM.cpp.i

CMakeFiles/FSM.dir/FSM.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/FSM.dir/FSM.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/abdul/CLionProjects/FSM/FSM.cpp -o CMakeFiles/FSM.dir/FSM.cpp.s

CMakeFiles/FSM.dir/FSM.cpp.o.requires:

.PHONY : CMakeFiles/FSM.dir/FSM.cpp.o.requires

CMakeFiles/FSM.dir/FSM.cpp.o.provides: CMakeFiles/FSM.dir/FSM.cpp.o.requires
	$(MAKE) -f CMakeFiles/FSM.dir/build.make CMakeFiles/FSM.dir/FSM.cpp.o.provides.build
.PHONY : CMakeFiles/FSM.dir/FSM.cpp.o.provides

CMakeFiles/FSM.dir/FSM.cpp.o.provides.build: CMakeFiles/FSM.dir/FSM.cpp.o


CMakeFiles/FSM.dir/FSMParser.cpp.o: CMakeFiles/FSM.dir/flags.make
CMakeFiles/FSM.dir/FSMParser.cpp.o: ../FSMParser.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/abdul/CLionProjects/FSM/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object CMakeFiles/FSM.dir/FSMParser.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/FSM.dir/FSMParser.cpp.o -c /home/abdul/CLionProjects/FSM/FSMParser.cpp

CMakeFiles/FSM.dir/FSMParser.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/FSM.dir/FSMParser.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/abdul/CLionProjects/FSM/FSMParser.cpp > CMakeFiles/FSM.dir/FSMParser.cpp.i

CMakeFiles/FSM.dir/FSMParser.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/FSM.dir/FSMParser.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/abdul/CLionProjects/FSM/FSMParser.cpp -o CMakeFiles/FSM.dir/FSMParser.cpp.s

CMakeFiles/FSM.dir/FSMParser.cpp.o.requires:

.PHONY : CMakeFiles/FSM.dir/FSMParser.cpp.o.requires

CMakeFiles/FSM.dir/FSMParser.cpp.o.provides: CMakeFiles/FSM.dir/FSMParser.cpp.o.requires
	$(MAKE) -f CMakeFiles/FSM.dir/build.make CMakeFiles/FSM.dir/FSMParser.cpp.o.provides.build
.PHONY : CMakeFiles/FSM.dir/FSMParser.cpp.o.provides

CMakeFiles/FSM.dir/FSMParser.cpp.o.provides.build: CMakeFiles/FSM.dir/FSMParser.cpp.o


# Object files for target FSM
FSM_OBJECTS = \
"CMakeFiles/FSM.dir/main.cpp.o" \
"CMakeFiles/FSM.dir/Command.cpp.o" \
"CMakeFiles/FSM.dir/State.cpp.o" \
"CMakeFiles/FSM.dir/ScopeManager.cpp.o" \
"CMakeFiles/FSM.dir/FSM.cpp.o" \
"CMakeFiles/FSM.dir/FSMParser.cpp.o"

# External object files for target FSM
FSM_EXTERNAL_OBJECTS =

FSM: CMakeFiles/FSM.dir/main.cpp.o
FSM: CMakeFiles/FSM.dir/Command.cpp.o
FSM: CMakeFiles/FSM.dir/State.cpp.o
FSM: CMakeFiles/FSM.dir/ScopeManager.cpp.o
FSM: CMakeFiles/FSM.dir/FSM.cpp.o
FSM: CMakeFiles/FSM.dir/FSMParser.cpp.o
FSM: CMakeFiles/FSM.dir/build.make
FSM: CMakeFiles/FSM.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/abdul/CLionProjects/FSM/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Linking CXX executable FSM"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/FSM.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/FSM.dir/build: FSM

.PHONY : CMakeFiles/FSM.dir/build

CMakeFiles/FSM.dir/requires: CMakeFiles/FSM.dir/main.cpp.o.requires
CMakeFiles/FSM.dir/requires: CMakeFiles/FSM.dir/Command.cpp.o.requires
CMakeFiles/FSM.dir/requires: CMakeFiles/FSM.dir/State.cpp.o.requires
CMakeFiles/FSM.dir/requires: CMakeFiles/FSM.dir/ScopeManager.cpp.o.requires
CMakeFiles/FSM.dir/requires: CMakeFiles/FSM.dir/FSM.cpp.o.requires
CMakeFiles/FSM.dir/requires: CMakeFiles/FSM.dir/FSMParser.cpp.o.requires

.PHONY : CMakeFiles/FSM.dir/requires

CMakeFiles/FSM.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/FSM.dir/cmake_clean.cmake
.PHONY : CMakeFiles/FSM.dir/clean

CMakeFiles/FSM.dir/depend:
	cd /home/abdul/CLionProjects/FSM/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/abdul/CLionProjects/FSM /home/abdul/CLionProjects/FSM /home/abdul/CLionProjects/FSM/cmake-build-debug /home/abdul/CLionProjects/FSM/cmake-build-debug /home/abdul/CLionProjects/FSM/cmake-build-debug/CMakeFiles/FSM.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/FSM.dir/depend

