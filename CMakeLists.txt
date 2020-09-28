cmake_minimum_required(VERSION 2.8)

project(stressMap)

include($ENV{DEVKIT_LOCATION}/cmake/pluginEntry.cmake)

set(PROJECT_NAME stressMap)
set(OpenGL_PATH /System/Library/Frameworks/OpenGL.framework)

set(SOURCE_FILES
    stressMap.h
    stressMap.cpp
    stressMapOverride.h
    stressMapOverride.cpp
    mainPlugin.cpp

)

set(LIBRARIES
    OpenMaya
    Foundation
    OpenMayaUI
    OpenMayaAnim
    OpenMayaRender
    OpenGL
)

build_plugin()
