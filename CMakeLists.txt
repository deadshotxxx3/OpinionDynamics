cmake_minimum_required(VERSION 3.12)
project(opinion_dynamics CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(opinion_dynamics ${SOURCES})
target_include_directories(opinion_dynamics PRIVATE include)

find_package(Doxygen OPTIONAL_COMPONENTS dot)
if(DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_LANGUAGE Russian)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
    set(DOXYGEN_MARKDOWN_ID_STYLE GITHUB)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
    set(DOXYGEN_STRIP_FROM_INC_PATH include)
    doxygen_add_docs(docs include README.md)
endif()