# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.20

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
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.20.3/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.20.3/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/jomof/projects/binja

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/jomof/projects/binja/debug-build

# Include any dependencies generated for this target.
include CMakeFiles/build_log_perftest.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/build_log_perftest.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/build_log_perftest.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/build_log_perftest.dir/flags.make

CMakeFiles/build_log_perftest.dir/src/build_log_perftest.cc.o: CMakeFiles/build_log_perftest.dir/flags.make
CMakeFiles/build_log_perftest.dir/src/build_log_perftest.cc.o: ../src/build_log_perftest.cc
CMakeFiles/build_log_perftest.dir/src/build_log_perftest.cc.o: CMakeFiles/build_log_perftest.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/jomof/projects/binja/debug-build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/build_log_perftest.dir/src/build_log_perftest.cc.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/build_log_perftest.dir/src/build_log_perftest.cc.o -MF CMakeFiles/build_log_perftest.dir/src/build_log_perftest.cc.o.d -o CMakeFiles/build_log_perftest.dir/src/build_log_perftest.cc.o -c /Users/jomof/projects/binja/src/build_log_perftest.cc

CMakeFiles/build_log_perftest.dir/src/build_log_perftest.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/build_log_perftest.dir/src/build_log_perftest.cc.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/jomof/projects/binja/src/build_log_perftest.cc > CMakeFiles/build_log_perftest.dir/src/build_log_perftest.cc.i

CMakeFiles/build_log_perftest.dir/src/build_log_perftest.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/build_log_perftest.dir/src/build_log_perftest.cc.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/jomof/projects/binja/src/build_log_perftest.cc -o CMakeFiles/build_log_perftest.dir/src/build_log_perftest.cc.s

# Object files for target build_log_perftest
build_log_perftest_OBJECTS = \
"CMakeFiles/build_log_perftest.dir/src/build_log_perftest.cc.o"

# External object files for target build_log_perftest
build_log_perftest_EXTERNAL_OBJECTS = \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/build_log.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/build.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/clean.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/clparser.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/dyndep.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/dyndep_parser.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/debug_flags.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/deps_log.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/disk_interface.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/edit_distance.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/eval_env.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/graph.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/graphviz.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/json.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/line_printer.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/manifest_parser.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/manifest_to_bin_parser.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/metrics.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/missing_deps.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/parser.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/state.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/status.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/string_piece_util.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/util.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/version.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja.dir/src/subprocess-posix.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja-re2c.dir/src/depfile_parser.cc.o" \
"/Users/jomof/projects/binja/debug-build/CMakeFiles/libninja-re2c.dir/src/lexer.cc.o"

build_log_perftest: CMakeFiles/build_log_perftest.dir/src/build_log_perftest.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/build_log.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/build.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/clean.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/clparser.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/dyndep.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/dyndep_parser.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/debug_flags.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/deps_log.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/disk_interface.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/edit_distance.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/eval_env.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/graph.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/graphviz.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/json.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/line_printer.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/manifest_parser.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/manifest_to_bin_parser.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/metrics.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/missing_deps.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/parser.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/state.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/status.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/string_piece_util.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/util.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/version.cc.o
build_log_perftest: CMakeFiles/libninja.dir/src/subprocess-posix.cc.o
build_log_perftest: CMakeFiles/libninja-re2c.dir/src/depfile_parser.cc.o
build_log_perftest: CMakeFiles/libninja-re2c.dir/src/lexer.cc.o
build_log_perftest: CMakeFiles/build_log_perftest.dir/build.make
build_log_perftest: CMakeFiles/build_log_perftest.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/jomof/projects/binja/debug-build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable build_log_perftest"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/build_log_perftest.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/build_log_perftest.dir/build: build_log_perftest
.PHONY : CMakeFiles/build_log_perftest.dir/build

CMakeFiles/build_log_perftest.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/build_log_perftest.dir/cmake_clean.cmake
.PHONY : CMakeFiles/build_log_perftest.dir/clean

CMakeFiles/build_log_perftest.dir/depend:
	cd /Users/jomof/projects/binja/debug-build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/jomof/projects/binja /Users/jomof/projects/binja /Users/jomof/projects/binja/debug-build /Users/jomof/projects/binja/debug-build /Users/jomof/projects/binja/debug-build/CMakeFiles/build_log_perftest.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/build_log_perftest.dir/depend

