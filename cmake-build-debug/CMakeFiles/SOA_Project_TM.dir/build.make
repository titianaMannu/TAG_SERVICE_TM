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
CMAKE_COMMAND = /snap/clion/162/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /snap/clion/162/bin/cmake/linux/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/tiziana/CLionProjects/SOA_Project_TM

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/tiziana/CLionProjects/SOA_Project_TM/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/SOA_Project_TM.dir/depend.make
# Include the progress variables for this target.
include CMakeFiles/SOA_Project_TM.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/SOA_Project_TM.dir/flags.make

CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_main.c.o: CMakeFiles/SOA_Project_TM.dir/flags.make
CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_main.c.o: ../tag_service/systbl_hack/systbl_hack_main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/tiziana/CLionProjects/SOA_Project_TM/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_main.c.o"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_main.c.o -c /home/tiziana/CLionProjects/SOA_Project_TM/tag_service/systbl_hack/systbl_hack_main.c

CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_main.c.i"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/tiziana/CLionProjects/SOA_Project_TM/tag_service/systbl_hack/systbl_hack_main.c > CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_main.c.i

CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_main.c.s"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/tiziana/CLionProjects/SOA_Project_TM/tag_service/systbl_hack/systbl_hack_main.c -o CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_main.c.s

CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_service.c.o: CMakeFiles/SOA_Project_TM.dir/flags.make
CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_service.c.o: ../tag_service/systbl_hack/systbl_hack_service.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/tiziana/CLionProjects/SOA_Project_TM/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_service.c.o"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_service.c.o -c /home/tiziana/CLionProjects/SOA_Project_TM/tag_service/systbl_hack/systbl_hack_service.c

CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_service.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_service.c.i"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/tiziana/CLionProjects/SOA_Project_TM/tag_service/systbl_hack/systbl_hack_service.c > CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_service.c.i

CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_service.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_service.c.s"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/tiziana/CLionProjects/SOA_Project_TM/tag_service/systbl_hack/systbl_hack_service.c -o CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_service.c.s

CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c.o: CMakeFiles/SOA_Project_TM.dir/flags.make
CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c.o: ../tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/tiziana/CLionProjects/SOA_Project_TM/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c.o"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c.o -c /home/tiziana/CLionProjects/SOA_Project_TM/tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c

CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c.i"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/tiziana/CLionProjects/SOA_Project_TM/tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c > CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c.i

CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c.s"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/tiziana/CLionProjects/SOA_Project_TM/tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c -o CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c.s

CMakeFiles/SOA_Project_TM.dir/tag_service/tag.c.o: CMakeFiles/SOA_Project_TM.dir/flags.make
CMakeFiles/SOA_Project_TM.dir/tag_service/tag.c.o: ../tag_service/tag.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/tiziana/CLionProjects/SOA_Project_TM/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object CMakeFiles/SOA_Project_TM.dir/tag_service/tag.c.o"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/SOA_Project_TM.dir/tag_service/tag.c.o -c /home/tiziana/CLionProjects/SOA_Project_TM/tag_service/tag.c

CMakeFiles/SOA_Project_TM.dir/tag_service/tag.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/SOA_Project_TM.dir/tag_service/tag.c.i"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/tiziana/CLionProjects/SOA_Project_TM/tag_service/tag.c > CMakeFiles/SOA_Project_TM.dir/tag_service/tag.c.i

CMakeFiles/SOA_Project_TM.dir/tag_service/tag.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/SOA_Project_TM.dir/tag_service/tag.c.s"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/tiziana/CLionProjects/SOA_Project_TM/tag_service/tag.c -o CMakeFiles/SOA_Project_TM.dir/tag_service/tag.c.s

CMakeFiles/SOA_Project_TM.dir/user/user-1.c.o: CMakeFiles/SOA_Project_TM.dir/flags.make
CMakeFiles/SOA_Project_TM.dir/user/user-1.c.o: ../user/user-1.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/tiziana/CLionProjects/SOA_Project_TM/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object CMakeFiles/SOA_Project_TM.dir/user/user-1.c.o"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/SOA_Project_TM.dir/user/user-1.c.o -c /home/tiziana/CLionProjects/SOA_Project_TM/user/user-1.c

CMakeFiles/SOA_Project_TM.dir/user/user-1.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/SOA_Project_TM.dir/user/user-1.c.i"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/tiziana/CLionProjects/SOA_Project_TM/user/user-1.c > CMakeFiles/SOA_Project_TM.dir/user/user-1.c.i

CMakeFiles/SOA_Project_TM.dir/user/user-1.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/SOA_Project_TM.dir/user/user-1.c.s"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/tiziana/CLionProjects/SOA_Project_TM/user/user-1.c -o CMakeFiles/SOA_Project_TM.dir/user/user-1.c.s

CMakeFiles/SOA_Project_TM.dir/tag_service/tag_main.c.o: CMakeFiles/SOA_Project_TM.dir/flags.make
CMakeFiles/SOA_Project_TM.dir/tag_service/tag_main.c.o: ../tag_service/tag_main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/tiziana/CLionProjects/SOA_Project_TM/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object CMakeFiles/SOA_Project_TM.dir/tag_service/tag_main.c.o"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/SOA_Project_TM.dir/tag_service/tag_main.c.o -c /home/tiziana/CLionProjects/SOA_Project_TM/tag_service/tag_main.c

CMakeFiles/SOA_Project_TM.dir/tag_service/tag_main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/SOA_Project_TM.dir/tag_service/tag_main.c.i"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/tiziana/CLionProjects/SOA_Project_TM/tag_service/tag_main.c > CMakeFiles/SOA_Project_TM.dir/tag_service/tag_main.c.i

CMakeFiles/SOA_Project_TM.dir/tag_service/tag_main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/SOA_Project_TM.dir/tag_service/tag_main.c.s"
	/usr/bin/gcc-9 $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/tiziana/CLionProjects/SOA_Project_TM/tag_service/tag_main.c -o CMakeFiles/SOA_Project_TM.dir/tag_service/tag_main.c.s

# Object files for target SOA_Project_TM
SOA_Project_TM_OBJECTS = \
"CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_main.c.o" \
"CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_service.c.o" \
"CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c.o" \
"CMakeFiles/SOA_Project_TM.dir/tag_service/tag.c.o" \
"CMakeFiles/SOA_Project_TM.dir/user/user-1.c.o" \
"CMakeFiles/SOA_Project_TM.dir/tag_service/tag_main.c.o"

# External object files for target SOA_Project_TM
SOA_Project_TM_EXTERNAL_OBJECTS =

SOA_Project_TM: CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_main.c.o
SOA_Project_TM: CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/systbl_hack_service.c.o
SOA_Project_TM: CMakeFiles/SOA_Project_TM.dir/tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c.o
SOA_Project_TM: CMakeFiles/SOA_Project_TM.dir/tag_service/tag.c.o
SOA_Project_TM: CMakeFiles/SOA_Project_TM.dir/user/user-1.c.o
SOA_Project_TM: CMakeFiles/SOA_Project_TM.dir/tag_service/tag_main.c.o
SOA_Project_TM: CMakeFiles/SOA_Project_TM.dir/build.make
SOA_Project_TM: CMakeFiles/SOA_Project_TM.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/tiziana/CLionProjects/SOA_Project_TM/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Linking C executable SOA_Project_TM"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/SOA_Project_TM.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/SOA_Project_TM.dir/build: SOA_Project_TM
.PHONY : CMakeFiles/SOA_Project_TM.dir/build

CMakeFiles/SOA_Project_TM.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/SOA_Project_TM.dir/cmake_clean.cmake
.PHONY : CMakeFiles/SOA_Project_TM.dir/clean

CMakeFiles/SOA_Project_TM.dir/depend:
	cd /home/tiziana/CLionProjects/SOA_Project_TM/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/tiziana/CLionProjects/SOA_Project_TM /home/tiziana/CLionProjects/SOA_Project_TM /home/tiziana/CLionProjects/SOA_Project_TM/cmake-build-debug /home/tiziana/CLionProjects/SOA_Project_TM/cmake-build-debug /home/tiziana/CLionProjects/SOA_Project_TM/cmake-build-debug/CMakeFiles/SOA_Project_TM.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/SOA_Project_TM.dir/depend

