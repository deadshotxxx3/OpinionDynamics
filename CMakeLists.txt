cmake_minimum_required(VERSION 3.10)
project(opinion_dynamics)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(opinion_dynamics
    src/main.cpp
    src/config/ManualGenConfig.cpp
    src/io/InputHelpers.cpp
    src/graph/Graph.cpp
    src/graph/GraphGenerator.cpp
)