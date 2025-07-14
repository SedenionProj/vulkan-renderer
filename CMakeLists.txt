cmake_minimum_required (VERSION 3.8)

set (CMAKE_CXX_STANDARD 17)

project (VkRenderer)

add_subdirectory (VkRenderer/vendor)



file(GLOB_RECURSE SOURCES 
    VkRenderer/src/*.cpp
)

add_executable(VkRenderer ${SOURCES})

target_include_directories (VkRenderer PUBLIC 
    ${CMAKE_SOURCE_DIR}/VkRenderer
    ${GLM_INCLUDE}
    ${STB_INCLUDE}
    ${TINYOBJECTLOADER_INCLUDE}
    ${SPIRV_CROSS_INCLUDE}
    ${IMGUI_INCLUDE}
	${IMGUI_INCLUDE}/backends
	${IMGUI_INCLUDE}/misc/cpp
)
target_link_libraries(VkRenderer 
    volk
    glfw
    spirv-cross-cpp
    imgui
)
target_precompile_headers(VkRenderer PRIVATE VkRenderer/src/pch.hpp)