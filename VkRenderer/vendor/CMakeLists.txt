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

# volk
#set(VOLK_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/volk PARENT_SCOPE)
if (WIN32)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()
add_subdirectory(volk)

# imgui
set (IMGUI_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/imgui PARENT_SCOPE)
file(GLOB IMGUI_SOURCE
	"imgui/*.cpp"

	"imgui/backends/imgui_impl_glfw.cpp"
	"imgui/backends/imgui_impl_vulkan.cpp"
)

add_library(imgui STATIC ${IMGUI_SOURCE})
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_VULKAN_USE_VOLK)
target_include_directories(imgui PUBLIC imgui)
target_link_libraries(imgui PRIVATE glfw volk)