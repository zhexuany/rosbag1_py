# FindPythonLibs.cmake - Compatibility shim for deprecated module
#
# This is a compatibility shim for the deprecated FindPythonLibs.cmake module
# that was removed in CMake 3.27. ROS Noetic's catkin may use the deprecated
# module, so we provide this shim to bridge modern CMake (FindPython3) with
# catkin's legacy expectations.
#
# This module provides:
#   PYTHONLIBS_FOUND       - have the Python libs been found
#   PYTHON_LIBRARIES       - path to the python library
#   PYTHON_INCLUDE_PATH    - path to where Python.h is found (deprecated)
#   PYTHON_INCLUDE_DIRS    - path to where Python.h is found
#   PYTHON_DEBUG_LIBRARIES - path to the debug library (deprecated)
#   PYTHONLIBS_VERSION_STRING - version of the Python libs found
#
# Copyright 2025 Zhexuan Yang
# Licensed under the Apache License, Version 2.0

# If variables are already set, use them
if(DEFINED PYTHON_LIBRARIES AND DEFINED PYTHONLIBS_FOUND)
    # Variables already set, nothing to do
    return()
endif()

# Otherwise, find Python3 using modern CMake
find_package(Python3 COMPONENTS Development)

if(Python3_Development_FOUND)
    # Map Python3 variables to legacy PythonLibs variables
    set(PYTHONLIBS_FOUND TRUE CACHE BOOL "Python libraries found" FORCE)
    set(PYTHON_LIBRARIES "${Python3_LIBRARIES}" CACHE STRING "Python libraries" FORCE)
    set(PYTHON_INCLUDE_DIRS "${Python3_INCLUDE_DIRS}" CACHE STRING "Python include directories" FORCE)
    set(PYTHON_INCLUDE_PATH "${Python3_INCLUDE_DIRS}" CACHE STRING "Python include path (deprecated)" FORCE)
    set(PYTHONLIBS_VERSION_STRING "${Python3_VERSION}" CACHE STRING "Python libraries version" FORCE)
    
    # Also set the non-cached versions for immediate use
    set(PYTHONLIBS_FOUND TRUE)
    set(PYTHON_LIBRARIES "${Python3_LIBRARIES}")
    set(PYTHON_INCLUDE_DIRS "${Python3_INCLUDE_DIRS}")
    set(PYTHON_INCLUDE_PATH "${Python3_INCLUDE_DIRS}")
    set(PYTHONLIBS_VERSION_STRING "${Python3_VERSION}")
    
    message(STATUS "FindPythonLibs (shim): Found Python ${PYTHONLIBS_VERSION_STRING} libraries")
else()
    set(PYTHONLIBS_FOUND FALSE CACHE BOOL "Python libraries found" FORCE)
    set(PYTHONLIBS_FOUND FALSE)
    
    if(PythonLibs_FIND_REQUIRED)
        message(FATAL_ERROR "FindPythonLibs (shim): Could not find Python libraries")
    elseif(NOT PythonLibs_FIND_QUIETLY)
        message(STATUS "FindPythonLibs (shim): Could not find Python libraries")
    endif()
endif()

