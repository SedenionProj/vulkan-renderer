# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# glm
set(GLM_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/glm PARENT_SCOPE)

# stb
set(STB_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/stb PARENT_SCOPE)

# tinyobjectloader
set(TINYOBJECTLOADER_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader PARENT_SCOPE)

# spirv_cross
set(SPIRV_CROSS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross PARENT_SCOPE)
add_subdirectory(spirv-cross)

#target_link_libraries(VkRenderer spirv-cross-c-shared)