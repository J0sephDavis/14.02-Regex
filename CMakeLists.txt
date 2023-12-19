cmake_minimum_required (VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 

project(regex
  LANGUAGES CXX
  VERSION 1.0.0
)
add_executable(regex
	src/regex.cc
)
target_include_directories(regex
	PRIVATE src
)
set_target_properties(regex PROPERTIES CXX_STANDARD 20)
target_compile_options(regex PRIVATE -Wall -Werror -Wextra -Wpedantic -Wshadow)
