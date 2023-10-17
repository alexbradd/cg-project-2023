cmake_minimum_required(VERSION 3.5)

file(STRINGS version.txt cg_project_VERSION)
message(STATUS "Building project")

project(cg_project LANGUAGES CXX VERSION ${cg_project_VERSION})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)
