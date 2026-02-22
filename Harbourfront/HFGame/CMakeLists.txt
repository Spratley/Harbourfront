include(${CMAKE_SOURCE_DIR}/YakuEngine/CMake/CommonFunctions.cmake)
yaku_executable(HFGame Harbourfront)
yaku_link(HFGame PRIVATE CG YakuEngine)

# Fix weird bug where PCH/HFGame_PCH.h isn't found?
yaku_include_path(HFGame PUBLIC "")