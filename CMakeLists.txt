# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Minimum supported ESP-IDF version
if(NOT "${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER_EQUAL "5.0")
    message(FATAL_ERROR "This project requires ESP-IDF v5.0 or later")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(diamant-v2)
