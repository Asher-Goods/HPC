# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.28

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
CMAKE_SOURCE_DIR = /home/asher/HPC/MPI_Assignment

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/asher/HPC/MPI_Assignment

# Include any dependencies generated for this target.
include CMakeFiles/run.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/run.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/run.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/run.dir/flags.make

CMakeFiles/run.dir/main.cpp.o: CMakeFiles/run.dir/flags.make
CMakeFiles/run.dir/main.cpp.o: main.cpp
CMakeFiles/run.dir/main.cpp.o: CMakeFiles/run.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/asher/HPC/MPI_Assignment/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/run.dir/main.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/run.dir/main.cpp.o -MF CMakeFiles/run.dir/main.cpp.o.d -o CMakeFiles/run.dir/main.cpp.o -c /home/asher/HPC/MPI_Assignment/main.cpp

CMakeFiles/run.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/run.dir/main.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/asher/HPC/MPI_Assignment/main.cpp > CMakeFiles/run.dir/main.cpp.i

CMakeFiles/run.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/run.dir/main.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/asher/HPC/MPI_Assignment/main.cpp -o CMakeFiles/run.dir/main.cpp.s

CMakeFiles/run.dir/TemperatureAnalysisParallel.cpp.o: CMakeFiles/run.dir/flags.make
CMakeFiles/run.dir/TemperatureAnalysisParallel.cpp.o: TemperatureAnalysisParallel.cpp
CMakeFiles/run.dir/TemperatureAnalysisParallel.cpp.o: CMakeFiles/run.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/asher/HPC/MPI_Assignment/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/run.dir/TemperatureAnalysisParallel.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/run.dir/TemperatureAnalysisParallel.cpp.o -MF CMakeFiles/run.dir/TemperatureAnalysisParallel.cpp.o.d -o CMakeFiles/run.dir/TemperatureAnalysisParallel.cpp.o -c /home/asher/HPC/MPI_Assignment/TemperatureAnalysisParallel.cpp

CMakeFiles/run.dir/TemperatureAnalysisParallel.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/run.dir/TemperatureAnalysisParallel.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/asher/HPC/MPI_Assignment/TemperatureAnalysisParallel.cpp > CMakeFiles/run.dir/TemperatureAnalysisParallel.cpp.i

CMakeFiles/run.dir/TemperatureAnalysisParallel.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/run.dir/TemperatureAnalysisParallel.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/asher/HPC/MPI_Assignment/TemperatureAnalysisParallel.cpp -o CMakeFiles/run.dir/TemperatureAnalysisParallel.cpp.s

# Object files for target run
run_OBJECTS = \
"CMakeFiles/run.dir/main.cpp.o" \
"CMakeFiles/run.dir/TemperatureAnalysisParallel.cpp.o"

# External object files for target run
run_EXTERNAL_OBJECTS =

build/run: CMakeFiles/run.dir/main.cpp.o
build/run: CMakeFiles/run.dir/TemperatureAnalysisParallel.cpp.o
build/run: CMakeFiles/run.dir/build.make
build/run: CMakeFiles/run.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/asher/HPC/MPI_Assignment/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable build/run"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/run.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/run.dir/build: build/run
.PHONY : CMakeFiles/run.dir/build

CMakeFiles/run.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/run.dir/cmake_clean.cmake
.PHONY : CMakeFiles/run.dir/clean

CMakeFiles/run.dir/depend:
	cd /home/asher/HPC/MPI_Assignment && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/asher/HPC/MPI_Assignment /home/asher/HPC/MPI_Assignment /home/asher/HPC/MPI_Assignment /home/asher/HPC/MPI_Assignment /home/asher/HPC/MPI_Assignment/CMakeFiles/run.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/run.dir/depend

