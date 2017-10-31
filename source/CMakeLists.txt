cmake_minimum_required(VERSION 3.8)


# ==============================================================================
# Project info

project(polishing-heuristics)


# ==============================================================================
# C++ standard

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
add_definitions(-DIL_STD)


# ==============================================================================
# Path to external dependencies

# CPLEX
if (NOT DEFINED CPLEX_PATH)
    set(CPLEX_PATH "/opt/ibm/ILOG/CPLEX_Studio1271")
endif()


# ==============================================================================
# Paths to search for headers

include_directories(
        ${PROJECT_SOURCE_DIR}/lib/
        ${CPLEX_PATH}/cplex/include/
        ${CPLEX_PATH}/concert/include/)


# ==============================================================================
# Paths to search for libraries

link_directories(
        ${CPLEX_PATH}/cplex/lib/x86-64_linux/static_pic/
        ${CPLEX_PATH}/concert/lib/x86-64_linux/static_pic/)


# ==============================================================================
# Libraries to link

set(CPLEX_LIBS concert ilocplex cplex)
set(OTHER_LIBS m pthread)


# ==============================================================================
# Source files

set(SOURCE_FILES
        src/main.cpp
        src/heuristic.h
        src/solution_pool.h src/solution_pool.cpp
        src/problem_data.h src/problem_data.cpp
        src/abort_callback.h src/abort_callback.cpp
        src/heuristic_callback.h src/heuristic_callback.cpp
        src/pool_callback.h src/pool_callback.cpp
        src/rothberg.h src/rothberg.cpp
        src/maravilha.h src/maravilha.cpp)


# ==============================================================================
# Targets

add_executable(itor ${SOURCE_FILES})
target_link_libraries(itor ${CPLEX_LIBS} ${OTHER_LIBS})
