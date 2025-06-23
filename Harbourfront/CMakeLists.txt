cmake_minimum_required(VERSION 3.10)
project(Harbourfront)

include(${CMAKE_SOURCE_DIR}/YakuEngine/CMake/CommonFunctions.cmake)
yaku_solution(Harbourfront)

# Add projects
# Make sure new projects & modules are included here
yaku_subproject_dir(YakuEngine/Core)
yaku_subproject_dir(YakuEngine/Modules/CG)
yaku_subproject_dir(YakuEngine/Engine)
yaku_subproject(HFGame)

# Set game to startup project
yaku_startup(HFGame)

# Set configurations
yaku_config_projects(HFGame CG YakuEngine YakuCore)