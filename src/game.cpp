#include "Game.h"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_log.h>
#include <iostream>
#include <VkBootstrap.h>

constexpr int kScreenWidth{ 640 };
constexpr int kScreenHeight{ 480 };

Game::Game() {
	// Init SDL
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("SDL_Init failed: %s\n", SDL_GetError());
		std::exit(EXIT_FAILURE);
	}
	// Create Window
	_window = SDL_CreateWindow("steorra", kScreenWidth, kScreenHeight, SDL_WINDOW_VULKAN);
	if (!_window) {
		SDL_Log("SDL_CreateWindow failed: %s\n", SDL_GetError());
		std::exit(EXIT_FAILURE);
	}
	// Vulkan Instance
	vkb::InstanceBuilder builder;
	auto instRet = builder.set_app_name("steorra")
		.request_validation_layers()
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();
	if (!instRet) {
		std::cerr << "vkb::PhysicalDeviceSelector failed: " << instRet.error().message() << "\n";
		std::exit(EXIT_FAILURE);
	}
	_instance = instRet.value();
	// Create Surface
	if (!SDL_Vulkan_CreateSurface(_window, _instance, NULL, &_surface)) {
		SDL_Log("SDL_Vulkan_CreateSurface failed: %s\n", SDL_GetError());
		std::exit(EXIT_FAILURE);
	}
	// Select Device
	auto selectResult = vkb::PhysicalDeviceSelector{ _instance }.set_minimum_version(1, 3)
		.set_required_features_13(VkPhysicalDeviceVulkan13Features{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
			.synchronization2 = true,
			.dynamicRendering = true
			})
		.set_required_features_12(VkPhysicalDeviceVulkan12Features{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
			.descriptorIndexing = true,
			.bufferDeviceAddress = true,
			})
			.set_surface(_surface)
		.select();
	if (!selectResult) {
		std::cerr << "vkb::PhysicalDeviceSelector failed: " << selectResult.error().message() << "\n";
		std::exit(EXIT_FAILURE);
	}
	vkb::PhysicalDevice selectedDevice = selectResult.value();
	// Build Device
	auto deviceBuildResult = vkb::DeviceBuilder{ selectedDevice }.build();
	if (!deviceBuildResult) {
		std::cerr << "vkb::DeviceBuilder failed: " << selectResult.error().message() << "\n";
		std::exit(EXIT_FAILURE);
	}
	_device = deviceBuildResult.value();
	// Create Swapchain
	auto swapchainBuildResult = vkb::SwapchainBuilder{ _device }
		//.use_default_format_selection()
		.set_desired_format(VkSurfaceFormatKHR{ .format = VK_FORMAT_B8G8R8A8_UNORM, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(kScreenWidth, kScreenHeight)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build();
	if (!swapchainBuildResult) {
		std::cerr << "vkb::SwapchainBuilder failed: " << swapchainBuildResult.error().message() << "\n";
		std::exit(EXIT_FAILURE);
	}
	_swapchain = swapchainBuildResult.value();
	const auto& swapchainImages = _swapchain.get_images().value();
	const auto& swapchainImageViews = _swapchain.get_image_views().value();
	assert(swapchainImages.size() == swapchainImageViews.size());
	for (int i = 0; i < swapchainImages.size(); i++) {
		_swapchainImages.push_back({swapchainImages[i], swapchainImageViews[i]});
	}
	// Create Queue
	_graphicsQueue = _device.get_queue(vkb::QueueType::graphics).value();
	_graphicsQueueFamilyIndex = _device.get_queue_index(vkb::QueueType::graphics).value();
	// Init command pools and buffers
	VkCommandPoolCreateInfo commandPoolInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = _graphicsQueueFamilyIndex
	};
	VkCommandBufferAllocateInfo cmdAllocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	for (auto& frame : _frames) {
		VK_CHECK_abort(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &frame._cmdPool));
		cmdAllocInfo.commandPool = frame._cmdPool;
		VK_CHECK_abort(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &frame._cmdBuffer));
	}
	// Create Sync structures
	VkFenceCreateInfo fenceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	VkSemaphoreCreateInfo semaphoreCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};
	for (auto& frame : _frames) {
		VK_CHECK_abort(vkCreateFence(_device, &fenceCreateInfo, nullptr, &frame._renderFence));
		VK_CHECK_abort(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &frame._swapchainSemaphore));
	}
	for (auto& image : _swapchainImages) {
		VK_CHECK_abort(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &image._renderSemaphore));
	}
}

Game::~Game() {
	vkDeviceWaitIdle(_device);
	for (const auto& frame : _frames) {
		vkDestroyCommandPool(_device, frame._cmdPool, nullptr);
		vkDestroyFence(_device, frame._renderFence, nullptr);
		vkDestroySemaphore(_device, frame._swapchainSemaphore, nullptr);
	}
	for (const auto& image : _swapchainImages) {
		vkDestroySemaphore(_device, image._renderSemaphore, nullptr);
		vkDestroyImageView(_device, image._imageView, nullptr);
	}
	vkb::destroy_swapchain(_swapchain);
	vkb::destroy_device(_device);
	vkb::destroy_surface(_instance, _surface);
	vkb::destroy_instance(_instance);
	SDL_DestroyWindow(_window);
	SDL_Quit();
}

void Game::Run() {
	while (true) {
		SDL_Event e{};
		while (SDL_PollEvent(&e) == true) {
			if (e.type == SDL_EVENT_QUIT) {
				return;
			}
		}
		Draw();
		_frameNumber++;
	}
}

VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask) {
	VkImageSubresourceRange subImage{
		.aspectMask = aspectMask,
		.baseMipLevel = 0,
		.levelCount = VK_REMAINING_MIP_LEVELS,
		.baseArrayLayer = 0,
		.layerCount = VK_REMAINING_ARRAY_LAYERS,
	};
	return subImage;
}


void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout) {
	VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	VkImageMemoryBarrier2 imageBarrier{ 
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
		.oldLayout = currentLayout,
		.newLayout = newLayout,
		.image = image,
		.subresourceRange = ImageSubresourceRange(aspectMask)
	};

	VkDependencyInfo depInfo{
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &imageBarrier,
	};

	vkCmdPipelineBarrier2(cmd, &depInfo);
}

VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore) {
	VkSemaphoreSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = semaphore,
		.value = 1,
		.stageMask = stageMask,
		.deviceIndex = 0,
	};
	return submitInfo;
}

VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer cmd) {
	VkCommandBufferSubmitInfo info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.commandBuffer = cmd,
		.deviceMask = 0,
	};
	return info;
}

VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo) {
	VkSubmitInfo2 info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = cmd,
	};
	if (waitSemaphoreInfo) {
		info.waitSemaphoreInfoCount = 1;
		info.pWaitSemaphoreInfos = waitSemaphoreInfo;
	}
	if (signalSemaphoreInfo) {
		info.signalSemaphoreInfoCount = 1;
		info.pSignalSemaphoreInfos = signalSemaphoreInfo;
	}
	return info;
}


void Game::Draw() {
	// Wait for previous frame to finish rendering
	uint64_t ONE_SECOND = 1'000'000'000;
	auto& frame = GetCurrentFrame();

	VK_CHECK_abort(vkWaitForFences(_device, 1, &frame._renderFence, true, ONE_SECOND));
	VK_CHECK_abort(vkResetFences(_device, 1, &frame._renderFence));

	uint32_t swapchainImageIndex;
	VK_CHECK_abort(vkAcquireNextImageKHR(_device, _swapchain, ONE_SECOND, frame._swapchainSemaphore, nullptr, &swapchainImageIndex));
	auto& image = _swapchainImages[swapchainImageIndex];
	VkCommandBuffer cmd = frame._cmdBuffer;
	VK_CHECK_abort(vkResetCommandBuffer(cmd, 0));
	VkCommandBufferBeginInfo cmdBufferBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	VK_CHECK_abort(vkBeginCommandBuffer(cmd, &cmdBufferBeginInfo));

	TransitionImage(cmd, image._image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
	float flash = std::abs(std::sin(_frameNumber / 120.f));
	VkClearColorValue clearValue{
		.float32 = { 0.0f, 0.0f, flash, 1.0f }
	};
	VkImageSubresourceRange clearRange = ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

	//clear image
	vkCmdClearColorImage(cmd, image._image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

	//make the swapchain image into presentable mode
	TransitionImage(cmd, image._image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK_abort(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdinfo = CommandBufferSubmitInfo(cmd);

	VkSemaphoreSubmitInfo waitInfo = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frame._swapchainSemaphore);
	VkSemaphoreSubmitInfo signalInfo = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, image._renderSemaphore);

	VkSubmitInfo2 submit = SubmitInfo(&cmdinfo, &signalInfo, &waitInfo);

	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
	VK_CHECK_abort(vkQueueSubmit2(_graphicsQueue, 1, &submit, frame._renderFence));

	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &image._renderSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &_swapchain.swapchain,
		.pImageIndices = &swapchainImageIndex,
	};
	VK_CHECK_abort(vkQueuePresentKHR(_graphicsQueue, &presentInfo));
}

Game::FrameData& Game::GetCurrentFrame() {
	return _frames[_frameNumber % FRAME_OVERLAP];
}
