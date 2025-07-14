#include "src/vulkan/imguiContext.hpp"
#include "src/vulkan/context.hpp"
#include "src/vulkan/device.hpp"
#include "src/vulkan/swapchain.hpp"
#include "src/window.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

static VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;
static VkAllocationCallbacks* g_Allocator = nullptr;
static ImGui_ImplVulkanH_Window g_MainWindowData;

static void check_vk_result(VkResult err)
{
	if (err == VK_SUCCESS)
		return;
	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	if (err < 0)
		abort();
}

static void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height)
{
	wd->Surface = surface;

	// Select Surface Format
	const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_SRGB };
	const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(Device::get()->getPhysicalDevice().getHandle(), wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

	// Select Present Mode

	VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR };

	wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(Device::get()->getPhysicalDevice().getHandle(), wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
	//printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

	// Create SwapChain, RenderPass, Framebuffer, etc.
	ImGui_ImplVulkanH_CreateOrResizeWindow(Context::get()->getInstance(),
		Device::get()->getPhysicalDevice().getHandle(), Device::getHandle(), wd,
		Device::get()->getPhysicalDevice().getQueueFamilyIndices().graphicsFamily.value(), g_Allocator, width, height, 2);
}

void createDescriptorPool() {
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
	};
	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 0;
	for (VkDescriptorPoolSize& pool_size : pool_sizes)
		pool_info.maxSets += pool_size.descriptorCount;
	pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;
	vkCreateDescriptorPool(Device::getHandle(), &pool_info, g_Allocator, &g_DescriptorPool);
}

Gui::Gui(std::shared_ptr<Window> window) {
	createDescriptorPool();

	// Create Framebuffers
	wd = &g_MainWindowData;
	
	SetupVulkanWindow(wd, window->getSwapchain()->m_surface, window->m_data.width, window->m_data.height);
	
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForVulkan(window->getHandle(), true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	//init_info.ApiVersion = VK_API_VERSION_1_3;              // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
	init_info.Instance = Context::get()->getInstance();
	init_info.PhysicalDevice = Device::get()->getPhysicalDevice().getHandle();
	init_info.Device = Device::getHandle();
	init_info.QueueFamily = Device::get()->getPhysicalDevice().getQueueFamilyIndices().graphicsFamily.value();
	init_info.Queue = Device::get()->getGraphicsQueue();
	init_info.PipelineCache = g_PipelineCache;
	init_info.DescriptorPool = g_DescriptorPool;
	init_info.RenderPass = wd->RenderPass;
	init_info.Subpass = 0;
	init_info.MinImageCount = 2;
	init_info.ImageCount = window->getSwapchain()->getSwapchainTexturesCount();
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = g_Allocator;
	init_info.CheckVkResultFn = check_vk_result;
	ImGui_ImplVulkan_Init(&init_info);
	
}

Gui::~Gui() {
	VkDevice device = Device::getHandle();
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	wd->Surface = VK_NULL_HANDLE;
	ImGui_ImplVulkanH_DestroyWindow(
		Context::get()->getInstance(),
		device,
		&g_MainWindowData,
		g_Allocator
	);


	vkDestroyDescriptorPool(device, g_DescriptorPool, g_Allocator);
	vkDestroyPipelineCache(device, g_PipelineCache, g_Allocator);
}

void Gui::begin() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}