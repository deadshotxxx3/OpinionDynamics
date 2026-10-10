cmake_minimum_required(VERSION 3.10)
project(opinion_dynamics)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

include_directories(include)

add_executable(opinion_dynamics ${SOURCES})

target_include_directories(opinion_dynamics PRIVATE include)
