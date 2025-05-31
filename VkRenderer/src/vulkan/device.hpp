#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

class Context;

class PhysicalDevice{

public:
	PhysicalDevice(std::shared_ptr<Context> ctx);
	~PhysicalDevice();

	VkPhysicalDevice getHandle() { return m_physicalDevice; }

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

private:
	void pickPhysicalDevice();
	
	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

private:
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

	const std::vector<const char*> m_deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	std::shared_ptr<Context> m_ctx;

	friend class Device;
};

class Device {
public:
	Device(std::shared_ptr<Context> ctx);
	~Device();

	static VkDevice getHandle() { return m_handle; }

	PhysicalDevice& getPhysicalDevice() { return m_physicalDevice; }

public:
	void createDevice();

	static VkDevice m_handle;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	PhysicalDevice m_physicalDevice;
};