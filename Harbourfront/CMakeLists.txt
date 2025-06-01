cmake_minimum_required(VERSION 3.6)
project(Harbourfront)

# Shared project settings
include(${CMAKE_SOURCE_DIR}/YakuEngine/CMake/CommonSettings.cmake)

# Add projects
# Make sure new projects are included here
add_subdirectory(YakuEngine ${CMAKE_SOURCE_DIR}/YakuEngine)
add_subdirectory(HFGame ${CMAKE_SOURCE_DIR}/HFGame)

# Set game to startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT HFGame)