# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/lhh/workspace/engine/3rd/msgpack-3.1.1

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/lhh/workspace/engine/3rd/msgpack-3.1.1

# Include any dependencies generated for this target.
include example/cpp03/CMakeFiles/custom.dir/depend.make

# Include the progress variables for this target.
include example/cpp03/CMakeFiles/custom.dir/progress.make

# Include the compile flags for this target's objects.
include example/cpp03/CMakeFiles/custom.dir/flags.make

example/cpp03/CMakeFiles/custom.dir/custom.cpp.o: example/cpp03/CMakeFiles/custom.dir/flags.make
example/cpp03/CMakeFiles/custom.dir/custom.cpp.o: example/cpp03/custom.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/lhh/workspace/engine/3rd/msgpack-3.1.1/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object example/cpp03/CMakeFiles/custom.dir/custom.cpp.o"
	cd /home/lhh/workspace/engine/3rd/msgpack-3.1.1/example/cpp03 && /usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/custom.dir/custom.cpp.o -c /home/lhh/workspace/engine/3rd/msgpack-3.1.1/example/cpp03/custom.cpp

example/cpp03/CMakeFiles/custom.dir/custom.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/custom.dir/custom.cpp.i"
	cd /home/lhh/workspace/engine/3rd/msgpack-3.1.1/example/cpp03 && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/lhh/workspace/engine/3rd/msgpack-3.1.1/example/cpp03/custom.cpp > CMakeFiles/custom.dir/custom.cpp.i

example/cpp03/CMakeFiles/custom.dir/custom.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/custom.dir/custom.cpp.s"
	cd /home/lhh/workspace/engine/3rd/msgpack-3.1.1/example/cpp03 && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/lhh/workspace/engine/3rd/msgpack-3.1.1/example/cpp03/custom.cpp -o CMakeFiles/custom.dir/custom.cpp.s

example/cpp03/CMakeFiles/custom.dir/custom.cpp.o.requires:

.PHONY : example/cpp03/CMakeFiles/custom.dir/custom.cpp.o.requires

example/cpp03/CMakeFiles/custom.dir/custom.cpp.o.provides: example/cpp03/CMakeFiles/custom.dir/custom.cpp.o.requires
	$(MAKE) -f example/cpp03/CMakeFiles/custom.dir/build.make example/cpp03/CMakeFiles/custom.dir/custom.cpp.o.provides.build
.PHONY : example/cpp03/CMakeFiles/custom.dir/custom.cpp.o.provides

example/cpp03/CMakeFiles/custom.dir/custom.cpp.o.provides.build: example/cpp03/CMakeFiles/custom.dir/custom.cpp.o


# Object files for target custom
custom_OBJECTS = \
"CMakeFiles/custom.dir/custom.cpp.o"

# External object files for target custom
custom_EXTERNAL_OBJECTS =

example/cpp03/custom: example/cpp03/CMakeFiles/custom.dir/custom.cpp.o
example/cpp03/custom: example/cpp03/CMakeFiles/custom.dir/build.make
example/cpp03/custom: example/cpp03/CMakeFiles/custom.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/lhh/workspace/engine/3rd/msgpack-3.1.1/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable custom"
	cd /home/lhh/workspace/engine/3rd/msgpack-3.1.1/example/cpp03 && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/custom.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
example/cpp03/CMakeFiles/custom.dir/build: example/cpp03/custom

.PHONY : example/cpp03/CMakeFiles/custom.dir/build

example/cpp03/CMakeFiles/custom.dir/requires: example/cpp03/CMakeFiles/custom.dir/custom.cpp.o.requires

.PHONY : example/cpp03/CMakeFiles/custom.dir/requires

example/cpp03/CMakeFiles/custom.dir/clean:
	cd /home/lhh/workspace/engine/3rd/msgpack-3.1.1/example/cpp03 && $(CMAKE_COMMAND) -P CMakeFiles/custom.dir/cmake_clean.cmake
.PHONY : example/cpp03/CMakeFiles/custom.dir/clean

example/cpp03/CMakeFiles/custom.dir/depend:
	cd /home/lhh/workspace/engine/3rd/msgpack-3.1.1 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/lhh/workspace/engine/3rd/msgpack-3.1.1 /home/lhh/workspace/engine/3rd/msgpack-3.1.1/example/cpp03 /home/lhh/workspace/engine/3rd/msgpack-3.1.1 /home/lhh/workspace/engine/3rd/msgpack-3.1.1/example/cpp03 /home/lhh/workspace/engine/3rd/msgpack-3.1.1/example/cpp03/CMakeFiles/custom.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : example/cpp03/CMakeFiles/custom.dir/depend

