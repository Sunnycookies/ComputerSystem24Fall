# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/student/workspace/code/lab2

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/student/workspace/code/lab2/build

# Include any dependencies generated for this target.
include CMakeFiles/sender.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/sender.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/sender.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/sender.dir/flags.make

CMakeFiles/sender.dir/src/sender.c.o: CMakeFiles/sender.dir/flags.make
CMakeFiles/sender.dir/src/sender.c.o: ../src/sender.c
CMakeFiles/sender.dir/src/sender.c.o: CMakeFiles/sender.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/student/workspace/code/lab2/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/sender.dir/src/sender.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/sender.dir/src/sender.c.o -MF CMakeFiles/sender.dir/src/sender.c.o.d -o CMakeFiles/sender.dir/src/sender.c.o -c /home/student/workspace/code/lab2/src/sender.c

CMakeFiles/sender.dir/src/sender.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/sender.dir/src/sender.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/student/workspace/code/lab2/src/sender.c > CMakeFiles/sender.dir/src/sender.c.i

CMakeFiles/sender.dir/src/sender.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/sender.dir/src/sender.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/student/workspace/code/lab2/src/sender.c -o CMakeFiles/sender.dir/src/sender.c.s

# Object files for target sender
sender_OBJECTS = \
"CMakeFiles/sender.dir/src/sender.c.o"

# External object files for target sender
sender_EXTERNAL_OBJECTS =

sender: CMakeFiles/sender.dir/src/sender.c.o
sender: CMakeFiles/sender.dir/build.make
sender: librtp_all.a
sender: CMakeFiles/sender.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/student/workspace/code/lab2/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable sender"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/sender.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/sender.dir/build: sender
.PHONY : CMakeFiles/sender.dir/build

CMakeFiles/sender.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/sender.dir/cmake_clean.cmake
.PHONY : CMakeFiles/sender.dir/clean

CMakeFiles/sender.dir/depend:
	cd /home/student/workspace/code/lab2/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/student/workspace/code/lab2 /home/student/workspace/code/lab2 /home/student/workspace/code/lab2/build /home/student/workspace/code/lab2/build /home/student/workspace/code/lab2/build/CMakeFiles/sender.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/sender.dir/depend

