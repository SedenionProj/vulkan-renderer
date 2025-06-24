cmake_minimum_required (VERSION 3.8)

set (CMAKE_CXX_STANDARD 17)

project (VkRenderer)

add_subdirectory (VkRenderer/vendor)



file(GLOB_RECURSE SOURCES 
    VkRenderer/src/*.cpp
)

add_executable(VkRenderer ${SOURCES})

find_package(Vulkan REQUIRED)
target_include_directories (VkRenderer PUBLIC 
    ${CMAKE_SOURCE_DIR}/VkRenderer
    ${VULKAN_INCLUDE_DIRS}
    ${GLM_INCLUDE}
    ${STB_INCLUDE}
    ${TINYOBJECTLOADER_INCLUDE}
    ${SPIRV_CROSS_INCLUDE}
)
target_link_libraries(VkRenderer 
    Vulkan::Vulkan
    glfw
    spirv-cross-cpp
)

target_precompile_headers(VkRenderer PRIVATE VkRenderer/src/pch.hpp)