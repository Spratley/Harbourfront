project(HFGame LANGUAGES CXX)

# Create the application
add_executable(HFGame)
set_target_properties(HFGame PROPERTIES OUTPUT_NAME "Harbourfront")

# Gather all code files
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/*.inl
)

# Generate filters for folders
foreach(source_file ${GAME_SOURCES})
	file(RELATIVE_PATH relative_path "${CMAKE_CURRENT_SOURCE_DIR}" "${source_file}")
	get_filename_component(source_path "${relative_path}" PATH)
	string(REPLACE "/" "\\" filter_path "${source_path}")
	source_group("${filter_path}" FILES "${source_file}")
endforeach()

target_sources(HFGame PUBLIC ${GAME_SOURCES})

# Link to engine lib
target_link_libraries(HFGame PRIVATE YakuEngine)

#Project Settings
include(${CMAKE_SOURCE_DIR}/YakuEngine/CMake/CommonSettings.cmake)