# Rename this file to x86-64-toolchain.cmake

# Set baseline x86-64 flags for maximum compatibility
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=x86-64" CACHE STRING "C flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64" CACHE STRING "C++ flags") 