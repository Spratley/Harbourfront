cmake_minimum_required(VERSION 3.10)
project(Harbourfront)

include(${CMAKE_SOURCE_DIR}/YakuEngine/CMake/CommonFunctions.cmake)
yaku_solution(Harbourfront)

# Add projects
# Make sure new projects are included here
yaku_subproject(YakuEngine)
yaku_subproject(HFGame)

# Set game to startup project
yaku_startup(HFGame)

# Set configurations
yaku_config_projects(HFGame YakuEngine)