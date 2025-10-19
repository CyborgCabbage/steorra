#pragma once
#include <SDL3/SDL_video.h>
#include <VkBootstrap.h>
#include <array>

const unsigned FRAME_OVERLAP = 2;

#define VK_CHECK_abort(x)\
	do {\
		VkResult err = x;\
		if (err) {\
			std::cout <<"Detected Vulkan error: " << err << std::endl;\
			abort();\
		}\
	} while (0)

class Game {
public:
	Game();
	~Game();
	void Run();
private:
	struct FrameData {
		VkCommandPool _cmdPool;
		VkCommandBuffer _cmdBuffer;
		VkSemaphore _swapchainSemaphore;
		VkFence _renderFence;
	};
	struct SwapChainData {
		VkImage _image;
		VkImageView _imageView;
		VkSemaphore _renderSemaphore;
	};
	void Draw();
	FrameData& GetCurrentFrame();
	SDL_Window* _window;
	vkb::Instance _instance;
	vkb::Device _device;
	vkb::Swapchain _swapchain;
	std::vector<SwapChainData> _swapchainImages;
	VkSurfaceKHR _surface;
	std::array<FrameData, FRAME_OVERLAP> _frames = {};
	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamilyIndex;
	int32_t _frameNumber = 0;
};

