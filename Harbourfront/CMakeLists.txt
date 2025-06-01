cmake_minimum_required(VERSION 3.20)
project(Harbourfront)

# Shared project settings
include(${CMAKE_SOURCE_DIR}/YakuEngine/CMake/CommonSettings.cmake)

# Add projects
# Make sure new projects are included here
add_subdirectory(YakuEngine)
add_subdirectory(HFGame)