cmake_minimum_required(VERSION 3.20)
project(ManiaReplayRenderer VERSION 0.5)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()
list(APPEND CMAKE_PREFIX_PATH "/usr/local")
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)
find_package(nfd REQUIRED PATHS /usr/local/lib/cmake/nfd)
message(STATUS "nfd found: ${nfd_FOUND}")
file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(mania-renderer ${SOURCES})
target_include_directories(mania-renderer PRIVATE src)

# find lzma
if(WIN32)
    find_package(LibLZMA REQUIRED)
    set(LZMA_LIB LibLZMA::LibLZMA)
    find_package(miniz CONFIG REQUIRED)
    set(MINIZ_LIB miniz::miniz)
    target_include_directories(mania-renderer PRIVATE ${MINIZ_INCLUDE_DIRS})
else()
    set(LZMA_LIB lzma)
    set(MINIZ_LIB miniz)
    # find miniz include dir
    find_path(MINIZ_INCLUDE_DIR miniz.h PATH_SUFFIXES miniz)
    if(MINIZ_INCLUDE_DIR)
        target_include_directories(mania-renderer PRIVATE ${MINIZ_INCLUDE_DIR})
    endif()
endif()

target_link_libraries(mania-renderer
    SFML::Graphics
    SFML::Window
    SFML::System
    ${LZMA_LIB}
    nfd::nfd
    ${MINIZ_LIB}
)

if(UNIX)
    target_link_libraries(mania-renderer wayland-client)
endif()
