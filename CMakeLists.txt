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
    ${VULKAN_INCLUDE_DIRS}
    ${GLM_INCLUDE}
)
target_link_libraries(VkRenderer 
    Vulkan::Vulkan
    glfw
)

add_custom_command(TARGET VkRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/VkRenderer/assets ${CMAKE_BINARY_DIR}/assets)

target_precompile_headers(VkRenderer PRIVATE VkRenderer/src/pch.hpp)