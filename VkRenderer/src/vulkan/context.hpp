#pragma once
#include "src/vulkan/vkHeader.hpp"

class Context {
public:
	~Context();

	static void create();

	void createInstance();


	static Context* get() { return s_context; }
	VkInstance getInstance() const { return m_instance; }
	std::shared_ptr<Device> getDevice() const { return m_device; }
	VkPipelineCache getPipelineCache() const { return m_pipelineCache; }

private:
	Context();
	

	bool checkValidationLayerSupport();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void setupDebugMessenger();
	void createPipelineCache();
	std::vector<const char*> getRequiredExtensions();

private:
	const std::vector<const char*> m_validationLayers = {
	"VK_LAYER_KHRONOS_validation"
	};

	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
	VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;

	std::shared_ptr<Device> m_device;
	static Context* s_context;
	

#ifdef NDEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = true;
#endif

};