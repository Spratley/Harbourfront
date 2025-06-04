include(${CMAKE_SOURCE_DIR}/YakuEngine/CMake/CommonFunctions.cmake)
yaku_executable(HFGame Harbourfront)
yaku_link(HFGame PRIVATE CG YakuEngine)