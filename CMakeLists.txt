cmake_minimum_required(VERSION 3.20)
project(ManiaReplayRenderer) 

set(CMAKE_CXX_STANDARD 17) 

list(APPEND CMAKE_PREFIX_PATH "/usr/local")

find_package( SFML 3 REQUIRED COMPONENTS Graphics Window System)
find_package(nfd REQUIRED PATHS /usr/local/lib/cmake/nfd)
message(STATUS "nfd found: ${nfd_FOUND}")

file(GLOB_RECURSE SOURCES src/*.cpp) 

add_executable(mania-renderer ${SOURCES})

target_include_directories(mania-renderer PRIVATE src)

# find lzma 
if(WIN32)
	find_package(LibLZMA REQUIRED) 
	set(LZMA_LIB LibLZMA::LibLZMA)
else()
	set(LZMA_LIB lzma)
endif()

target_link_libraries(mania-renderer
	SFML::Graphics 
	SFML::Window
	SFML::System
	&{LZMA_LIB}
	nfd::nfd

) 

if(UNIX)
	target_link_libraries(mania-renderer wayland-client)
endif()
