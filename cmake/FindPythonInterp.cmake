# FindPythonInterp.cmake - Compatibility shim for deprecated module
#
# This is a compatibility shim for the deprecated FindPythonInterp.cmake module
# that was removed in CMake 3.27. ROS Noetic's catkin still uses the deprecated
# module, so we provide this shim to bridge modern CMake (FindPython3) with
# catkin's legacy expectations.
#
# This module provides:
#   PYTHONINTERP_FOUND - Was the Python executable found
#   PYTHON_EXECUTABLE  - path to the Python interpreter
#   PYTHON_VERSION_STRING - Python version found e.g. 2.5.2
#   PYTHON_VERSION_MAJOR  - Python major version found e.g. 2
#   PYTHON_VERSION_MINOR  - Python minor version found e.g. 5
#   PYTHON_VERSION_PATCH  - Python patch version found e.g. 2
#
# Copyright 2025 Zhexuan Yang
# Licensed under the Apache License, Version 2.0

# If variables are already set (e.g., by parent CMakeLists.txt), use them
if(DEFINED PYTHON_EXECUTABLE AND DEFINED PYTHONINTERP_FOUND)
    # Variables already set, nothing to do
    return()
endif()

# Otherwise, find Python3 using modern CMake
find_package(Python3 COMPONENTS Interpreter)

if(Python3_Interpreter_FOUND)
    # Map Python3 variables to legacy PythonInterp variables
    set(PYTHONINTERP_FOUND TRUE CACHE BOOL "Python interpreter found" FORCE)
    set(PYTHON_EXECUTABLE "${Python3_EXECUTABLE}" CACHE FILEPATH "Path to Python interpreter" FORCE)
    set(PYTHON_VERSION_STRING "${Python3_VERSION}" CACHE STRING "Python version" FORCE)
    set(PYTHON_VERSION_MAJOR "${Python3_VERSION_MAJOR}" CACHE STRING "Python major version" FORCE)
    set(PYTHON_VERSION_MINOR "${Python3_VERSION_MINOR}" CACHE STRING "Python minor version" FORCE)
    set(PYTHON_VERSION_PATCH "${Python3_VERSION_PATCH}" CACHE STRING "Python patch version" FORCE)
    
    # Also set the non-cached versions for immediate use
    set(PYTHONINTERP_FOUND TRUE)
    set(PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")
    set(PYTHON_VERSION_STRING "${Python3_VERSION}")
    set(PYTHON_VERSION_MAJOR "${Python3_VERSION_MAJOR}")
    set(PYTHON_VERSION_MINOR "${Python3_VERSION_MINOR}")
    set(PYTHON_VERSION_PATCH "${Python3_VERSION_PATCH}")
    
    message(STATUS "FindPythonInterp (shim): Found Python ${PYTHON_VERSION_STRING} at ${PYTHON_EXECUTABLE}")
else()
    set(PYTHONINTERP_FOUND FALSE CACHE BOOL "Python interpreter found" FORCE)
    set(PYTHONINTERP_FOUND FALSE)
    
    if(PythonInterp_FIND_REQUIRED)
        message(FATAL_ERROR "FindPythonInterp (shim): Could not find Python interpreter")
    elseif(NOT PythonInterp_FIND_QUIETLY)
        message(STATUS "FindPythonInterp (shim): Could not find Python interpreter")
    endif()
endif()

