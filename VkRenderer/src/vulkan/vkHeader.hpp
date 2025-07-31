#pragma once

#define VK_NO_PROTOTYPES
#define IMGUI_IMPL_VULKAN_USE_VOLK
#include <volk.h>

#define MAX_FRAMES_IN_FLIGHT 2

#define ASSETS_PATH "../VkRenderer/assets/"

//#define new new(_CLIENT_BLOCK,__FILE__, __LINE__)

inline const char* vkResultToString(VkResult result) {
    switch (result) {
        case VK_SUCCESS: return "VK_SUCCESS";
		case VK_NOT_READY: return "VK_NOT_READY";
        case VK_TIMEOUT: return "VK_TIMEOUT";
        case VK_EVENT_SET: return "VK_EVENT_SET";
        case VK_EVENT_RESET: return "VK_EVENT_RESET";
        case VK_INCOMPLETE: return "VK_INCOMPLETE";
		case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
        default: return "Unknown VkResult";
    }
}

#if 1
	#if 1
		#define DEBUG_ERROR(fmt, ...) \
				    {printf("ERROR: " fmt " [%s:%d]\n", ##__VA_ARGS__, __FILE__, __LINE__);\
					std::exit(EXIT_FAILURE); }
		
		#define DEBUG_MSG(fmt, ...) \
				    printf("MESSAGE: " fmt " [%s:%d]\n", ##__VA_ARGS__, __FILE__, __LINE__)
		
		#define DEBUG_WARNING(fmt, ...) \
				    printf("WARNING: " fmt " [%s:%d]\n", ##__VA_ARGS__, __FILE__, __LINE__)
	#endif
	
	#define DEBUG_ASSERT(x, fmt, ...) \
		    if (!(x)) DEBUG_ERROR("Assertion failed: " fmt, ##__VA_ARGS__)
	
	#define VK_CHECK(x)\
	do {\
		VkResult err = x;\
		if (err != VK_SUCCESS) {\
			DEBUG_ERROR("VK call failed: %s (%d)",\
				vkResultToString(err), err);\
		}\
	} while (0)
		
	#else
		#define DEBUG_ERROR(Message, ...)
		#define DEBUG_MSG(Message, ...)
		#define DEBUG_WARNING(Message, ...)
		#define DEBUG_ASSERT(x, ...)
#endif

class Texture;
class Texture2D;

class Window;
class Context;
class Device;
class CommandBuffer;
class CommandPool;
class Shader;
class DescriptorSet;
class Fence;
class Semaphore;
class Framebuffer;
class RenderPass;
class Pipeline;
class Swapchain;
class VertexBuffer;
class IndexBuffer;
class Material;

enum class TextureType {
	NONE,
	COLOR,
	DEPTH,
	CUBEMAP,
	SWAPCHAIN
};