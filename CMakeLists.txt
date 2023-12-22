cmake_minimum_required (VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 

project(regex
  LANGUAGES CXX
  VERSION 1.1.0
)

Include(FetchContent)
FetchContent_Declare(
	Catch2
	GIT_REPOSITORY https://github.com/catchorg/Catch2.git
	GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(Catch2)
add_executable(regex
	src/regex.cc
)
target_include_directories(regex
	PRIVATE src
)
option(PRINT_MESSAGES "Print debug information" OFF)
option(UNIT_TEST"enable unit testing" OFF)
if (UNIT_TEST)
	message("regex:: unit testing enabled")
	target_compile_definitions(regex PRIVATE UNIT_TESTING)
endif()
if (PRINT_MESSAGES)
	message("regex:: printing enabled")
	target_compile_definitions(regex PRIVATE PRINT_MESSAGES=1)
else()
	message("regex:: printing disabled")
	target_compile_definitions(regex PRIVATE PRINT_MESSAGES=0)
endif()
target_link_libraries(regex PRIVATE Catch2::Catch2WithMain)
set_target_properties(regex PROPERTIES CXX_STANDARD 20)
target_compile_options(regex PRIVATE -Wall -Werror -Wextra -Wpedantic -Wshadow)
