#pragma once
#include "src/vulkan/vkHeader.hpp"

class Window;
class ImGui_ImplVulkanH_Window;
class ImDrawData;

class Gui {
public:
	Gui(std::shared_ptr<Window> window);
	~Gui();

	void begin();

	ImGui_ImplVulkanH_Window* wd = nullptr;
};