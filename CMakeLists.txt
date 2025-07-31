cmake_minimum_required (VERSION 3.10)

set (CMAKE_CXX_STANDARD 17)

project (VkRenderer)

add_subdirectory (VkRenderer/vendor)

file(GLOB_RECURSE SOURCES 
    VkRenderer/src/*.cpp
)

add_executable(VkRendererApp ${SOURCES})

target_include_directories (VkRendererApp PUBLIC 
    ${CMAKE_SOURCE_DIR}/VkRenderer
    ${GLM_INCLUDE}
    ${STB_INCLUDE}
    ${TINYOBJECTLOADER_INCLUDE}
    ${SPIRV_CROSS_INCLUDE}
    ${IMGUI_INCLUDE}
	${IMGUI_INCLUDE}/backends
	${IMGUI_INCLUDE}/misc/cpp
)
target_link_libraries(VkRendererApp 
    volk
    glfw
    spirv-cross-cpp
    imgui
)

if(WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
elseif(UNIX)
    add_definitions(-DPLATFORM_LINUX)
endif()

target_precompile_headers(VkRendererApp PRIVATE VkRenderer/src/pch.hpp)