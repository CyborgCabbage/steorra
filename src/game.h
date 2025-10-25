#pragma once
#include <SDL3/SDL_video.h>
#include <VkBootstrap.h>
#include <array>
#include <deque>
#include <functional>
#include <vma/vk_mem_alloc.h>
#include <graphics/graphics_memory.h>
#include <graphics/graphics_shaders.h>

const unsigned FRAME_OVERLAP = 2;

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
		DeletionQueue _deletionQueue;
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
	DeletionQueue _mainDeletionQueue;
	VmaAllocator _allocator;
	struct AllocatedImage {
		VkImage _image;
		VkImageView _imageView;
		VmaAllocation _allocation;
		VkExtent3D _imageExtent;
		VkFormat _imageFormat;
	};
	AllocatedImage _drawImage;
	DescriptorAllocator _globalDescriptorAllocator;
	VkDescriptorSet _drawImageDescriptors;
	VkDescriptorSetLayout _drawImageDescriptorLayout;
	VkPipeline _gradientPipeline;
	VkPipelineLayout _gradientPipelineLayout;
	//Immediate Submit
	VkFence _immediateFence;
	VkCommandBuffer _immediateCommandBuffer;
	VkCommandPool _immediateCommandPool;
	void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
	void InitImgui();
	void DrawImgui(VkCommandBuffer cmd, VkImageView targetImageView);
};

