# DIAMANT v2.1 REV E - CMake Project Configuration
cmake_minimum_required(VERSION 3.16)

# Set project name and version
set(PROJECT_NAME "diamant-fusion")
set(PROJECT_VER "2.1.0")

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(${PROJECT_NAME})
