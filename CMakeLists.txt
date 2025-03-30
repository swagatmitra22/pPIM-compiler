cmake_minimum_required(VERSION 3.13)
project(pPIM_Compiler)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Add LLVM includes and definitions
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Source files
file(GLOB_RECURSE FRONTEND_SOURCES "src/frontend/*.cpp")
file(GLOB_RECURSE MIDDLE_END_SOURCES "src/middle_end/*.cpp")
file(GLOB_RECURSE BACKEND_SOURCES "src/backend/*.cpp")
file(GLOB_RECURSE SUPPORT_SOURCES "src/support/*.cpp")

# Main executable
add_executable(pPIM_compiler 
    src/main.cpp
    ${FRONTEND_SOURCES}
    ${MIDDLE_END_SOURCES}
    ${BACKEND_SOURCES}
    ${SUPPORT_SOURCES}
)

# Link against LLVM libraries
llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(pPIM_compiler ${llvm_libs})
