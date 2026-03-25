cmake_minimum_required(VERSION 3.20)
project(ManiaReplayRenderer) 

set(CMAKE_CXX_STANDARD 17) 

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System) 

file(GLOB_RECURSE SOURCES src/*.cpp) 

add_executable(mania-renderer ${SOURCES})

target_include_directories(mania-renderer PRIVATE src)

target_link_libraries(mania-renderer
	SFML::Graphics 
	SFML::Window
	SFML::System
	lzma
) 
