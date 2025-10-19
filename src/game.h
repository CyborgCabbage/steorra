#pragma once
#include <SDL3/SDL_video.h>
#include <VkBootstrap.h>

class Game {
public:
	Game();
	~Game();
	void Run();
private:
	// Platform
	SDL_Window* _window;
	vkb::Instance _instance;
	vkb::Device _device;
	vkb::Swapchain _swapchain;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	VkSurfaceKHR _surface;
};

