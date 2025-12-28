#pragma once
#include <SDL3/SDL_video.h>
#include <VkBootstrap.h>
#include <array>
#include <deque>
#include <functional>
#include <filesystem>
#include "graphics/graphics_types.h"
#include "graphics/graphics_memory.h"
#include "graphics/graphics_shaders.h"

const unsigned FRAME_OVERLAP = 2;

class Game {
public:
	Game();
	~Game();
	void Run();
private:
	struct FrameData {
		VkCommandPool cmdPool;
		VkCommandBuffer cmdBuffer;
		VkSemaphore swapchainSemaphore;
		VkFence renderFence;
		DeletionQueue deletionQueue;
	};
	struct SwapChainData {
		VkImage image;
		VkImageView imageView;
		VkSemaphore renderSemaphore;
	};
	void Draw();
	void DrawBackground(VkCommandBuffer cmd);
	void DrawGeometry(VkCommandBuffer cmd);
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
		VkImage image;
		VkImageView imageView;
		VmaAllocation allocation;
		VkExtent3D imageExtent;
		VkFormat imageFormat;
	};
	AllocatedImage _drawImage;
	AllocatedImage _depthImage;
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
	VkPipelineLayout _meshPipelineLayout;
	VkPipeline _meshPipeline;
	AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void DestroyBuffer(const AllocatedBuffer& buffer);
	GPUMeshBuffers UploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);
	bool LoadMeshes(const std::string& filePath);
	std::vector<MeshAsset> _meshes;
};

