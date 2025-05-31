#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Context {
public:
	Context();
	~Context();

	void createInstance();

	VkInstance getInstance() { return m_instance; }

private:
	bool checkValidationLayerSupport();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void setupDebugMessenger();
	std::vector<const char*> getRequiredExtensions();

private:
	const std::vector<const char*> m_validationLayers = {
	"VK_LAYER_KHRONOS_validation"
	};

	VkInstance m_instance;

	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif
};