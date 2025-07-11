#pragma once
#include "src/vulkan/vkHeader.hpp"
#include "src/vulkan/context.hpp"

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
	PhysicalDevice();
	~PhysicalDevice();

	VkPhysicalDevice getHandle() const { return m_physicalDevice; }

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat findDepthFormat();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

private:
	void pickPhysicalDevice();
	
	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

private:
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

	const std::vector<const char*> m_deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	friend class Device;
};

class Device {
public:
	Device();
	~Device();

	static std::shared_ptr<Device> get() { return Context::get()->getDevice(); }
	static VkDevice getHandle() { return Context::get()->getDevice()->m_handle; }

	VkDevice getDevice() { return m_handle; }
	VkQueue getGraphicsQueue() { return m_presentQueue; }
	VkQueue getPresentQueue() { return m_graphicsQueue; }
	PhysicalDevice& getPhysicalDevice() { return m_physicalDevice; }

private:
	void createDevice();

	VkDevice m_handle;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	PhysicalDevice m_physicalDevice;
};