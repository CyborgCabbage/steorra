#include "Game.h"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_log.h>
#include <iostream>
#include <VkBootstrap.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp> 
#include <glm/gtc/matrix_transform.hpp>
#include <graphics/graphics_data.h>
#include <graphics/graphics_command.h>
#include <graphics/graphics_errors.h>
#include <graphics/graphics_shaders.h>
#include <graphics/graphics_pipeline.h>

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
	// Memory allocator
	VmaAllocatorCreateInfo allocatorInfo = {
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = _device.physical_device,
		.device = _device,
		.instance = _instance,
	};
	vmaCreateAllocator(&allocatorInfo, &_allocator);
	_mainDeletionQueue.PushFunction([&]() {
		vmaDestroyAllocator(_allocator);
	});
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
	//draw image size will match the window
	VkExtent3D drawImageExtent = ToExtent3D(_swapchain.extent);

	//hardcoding the draw format to 16 bit float
	_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	_drawImage.imageExtent = drawImageExtent;

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageCreateInfo rimgInfo = ImageCreateInfo(_drawImage.imageFormat, drawImageUsages, drawImageExtent);

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo rimgAllocInfo = {};
	rimgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rimgAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(_allocator, &rimgInfo, &rimgAllocInfo, &_drawImage.image, &_drawImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo drawImageViewCreateInfo = ImageViewCreateInfo(_drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
	VK_CHECK_abort(vkCreateImageView(_device, &drawImageViewCreateInfo, nullptr, &_drawImage.imageView));

	//add to deletion queues
	_mainDeletionQueue.PushFunction([=]() {
		vkDestroyImageView(_device, _drawImage.imageView, nullptr);
		vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);
	});
	// Create Queue
	_graphicsQueue = _device.get_queue(vkb::QueueType::graphics).value();
	_graphicsQueueFamilyIndex = _device.get_queue_index(vkb::QueueType::graphics).value();
	// Init command pools and buffers
	VkCommandPoolCreateInfo commandPoolInfo = CommandPoolCreateInfo(_graphicsQueueFamilyIndex);
	for (auto& frame : _frames) {
		VK_CHECK_abort(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &frame.cmdPool));
		const auto allocateInfo = CommandBufferAllocateInfo(frame.cmdPool);
		VK_CHECK_abort(vkAllocateCommandBuffers(_device, &allocateInfo, &frame.cmdBuffer));
	}
	VK_CHECK_abort(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_immediateCommandPool));

	// allocate the command buffer for immediate submits
	const auto allocateInfo = CommandBufferAllocateInfo(_immediateCommandPool);
	VK_CHECK_abort(vkAllocateCommandBuffers(_device, &allocateInfo, &_immediateCommandBuffer));

	_mainDeletionQueue.PushFunction([=]() {
		vkDestroyCommandPool(_device, _immediateCommandPool, nullptr);
	});
	// Create Sync structures
	VkFenceCreateInfo fenceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	VkSemaphoreCreateInfo semaphoreCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};
	for (auto& frame : _frames) {
		VK_CHECK_abort(vkCreateFence(_device, &fenceCreateInfo, nullptr, &frame.renderFence));
		VK_CHECK_abort(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &frame.swapchainSemaphore));
	}
	for (auto& image : _swapchainImages) {
		VK_CHECK_abort(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &image.renderSemaphore));
	}
	VK_CHECK_abort(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_immediateFence));
	_mainDeletionQueue.PushFunction([=]() { vkDestroyFence(_device, _immediateFence, nullptr); });

	// Create Descriptors
	std::vector<DescriptorAllocator::PoolSizeRatio> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
	};

	_globalDescriptorAllocator.InitPool(_device, 10, sizes);

	// Make the descriptor set layout for our compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		_drawImageDescriptorLayout = builder.Build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
	}

	//allocate a descriptor set for our draw image
	_drawImageDescriptors = _globalDescriptorAllocator.Allocate(_device, _drawImageDescriptorLayout);

	VkDescriptorImageInfo imgInfo{};
	imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imgInfo.imageView = _drawImage.imageView;

	VkWriteDescriptorSet drawImageWrite = {};
	drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	drawImageWrite.dstBinding = 0;
	drawImageWrite.dstSet = _drawImageDescriptors;
	drawImageWrite.descriptorCount = 1;
	drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	drawImageWrite.pImageInfo = &imgInfo;

	vkUpdateDescriptorSets(_device, 1, &drawImageWrite, 0, nullptr);

	//make sure both the descriptor allocator and the new layout get cleaned up properly
	_mainDeletionQueue.PushFunction([&]() {
		_globalDescriptorAllocator.DestroyPool(_device);
		vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr);
	});
	// Compute Pipeline
	VkPushConstantRange pushConstantRange{
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.offset = 0,
		.size = sizeof(ComputePushConstants),
	};
	VkPipelineLayoutCreateInfo computeLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &_drawImageDescriptorLayout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstantRange,
	};
	VK_CHECK_abort(vkCreatePipelineLayout(_device, &computeLayoutInfo, nullptr, &_gradientPipelineLayout));
	VkShaderModule computeDrawShader;
	if (!LoadShaderModule("gradient.comp", _device, &computeDrawShader)) {
		std::cout << "Error when building the compute shader \n";
		std::exit(EXIT_FAILURE);
	}

	VkPipelineShaderStageCreateInfo stageinfo{};
	stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageinfo.module = computeDrawShader;
	stageinfo.pName = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.layout = _gradientPipelineLayout;
	computePipelineCreateInfo.stage = stageinfo;

	VK_CHECK_abort(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &_gradientPipeline));

	vkDestroyShaderModule(_device, computeDrawShader, nullptr);

	_mainDeletionQueue.PushFunction([&]() {
		vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
		vkDestroyPipeline(_device, _gradientPipeline, nullptr);
	});

	// Triangle Pipeline
	VkShaderModule triangleFragShader;
	if (!LoadShaderModule("colored_triangle.frag", _device, &triangleFragShader)) {
		std::cout << "Error when building the triangle fragment shader module \n";
		std::exit(EXIT_FAILURE);
	}

	VkShaderModule triangleVertShader;
	if (!LoadShaderModule("colored_triangle.vert", _device, &triangleVertShader)) {
		std::cout << "Error when building the triangle vertex shader module \n";
		std::exit(EXIT_FAILURE);
	}

	//build the pipeline layout that controls the inputs/outputs of the shader
	VkPushConstantRange vertexPushConstantRange{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = sizeof(VertexPushConstants),
	};
	 
	
	//we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
	VkPipelineLayoutCreateInfo triangleLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &vertexPushConstantRange,
	};
	VK_CHECK_abort(vkCreatePipelineLayout(_device, &triangleLayoutInfo, nullptr, &_trianglePipelineLayout));

	PipelineBuilder pipelineBuilder;
	pipelineBuilder._pipelineLayout = _trianglePipelineLayout;
	pipelineBuilder.SetShaders(triangleVertShader, triangleFragShader);
	pipelineBuilder.SetColorAttachmentFormat(_drawImage.imageFormat);
	// Leaving depth undefined
	_trianglePipeline = pipelineBuilder.BuildPipeline(_device);
	vkDestroyShaderModule(_device, triangleFragShader, nullptr);
	vkDestroyShaderModule(_device, triangleVertShader, nullptr);

	_mainDeletionQueue.PushFunction([&]() {
		vkDestroyPipelineLayout(_device, _trianglePipelineLayout, nullptr);
		vkDestroyPipeline(_device, _trianglePipeline, nullptr);
	});

	InitImgui();
}

Game::~Game() {
	vkDeviceWaitIdle(_device);
	for (auto& frame : _frames) {
		vkDestroyCommandPool(_device, frame.cmdPool, nullptr);
		vkDestroyFence(_device, frame.renderFence, nullptr);
		vkDestroySemaphore(_device, frame.swapchainSemaphore, nullptr);
		frame.deletionQueue.Flush();
	}
	_mainDeletionQueue.Flush();
	for (const auto& image : _swapchainImages) {
		vkDestroySemaphore(_device, image.renderSemaphore, nullptr);
		vkDestroyImageView(_device, image.imageView, nullptr);
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
			ImGui_ImplSDL3_ProcessEvent(&e);
		}
		// imgui new frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		//some imgui UI to test
		ImGui::ShowDemoWindow();

		//make imgui calculate internal draw structures
		ImGui::Render();
		Draw();
		_frameNumber++;
	}
}

void Game::Draw() {
	// Wait for previous frame to finish rendering
	uint64_t ONE_SECOND = 1'000'000'000;
	auto& frame = GetCurrentFrame();

	VK_CHECK_abort(vkWaitForFences(_device, 1, &frame.renderFence, true, ONE_SECOND));
	frame.deletionQueue.Flush();
	VK_CHECK_abort(vkResetFences(_device, 1, &frame.renderFence));

	uint32_t swapchainImageIndex;
	VK_CHECK_abort(vkAcquireNextImageKHR(_device, _swapchain, ONE_SECOND, frame.swapchainSemaphore, nullptr, &swapchainImageIndex));
	auto& image = _swapchainImages[swapchainImageIndex];
	VkCommandBuffer cmd = frame.cmdBuffer;
	VK_CHECK_abort(vkResetCommandBuffer(cmd, 0));
	VkCommandBufferBeginInfo cmdBufferBeginInfo = CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK_abort(vkBeginCommandBuffer(cmd, &cmdBufferBeginInfo));

	// transition our main draw image into general layout so we can write into it
	// we will overwrite it all so we dont care about what was the older layout
	TransitionImage(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	DrawBackground(cmd);

	TransitionImage(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	DrawGeometry(cmd);

	//transition the draw image and the swapchain image into their correct transfer layouts
	TransitionImage(cmd, _drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	TransitionImage(cmd, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// execute a copy from the draw image into the swapchain
	CopyImageToImage(cmd, _drawImage.image, image.image, ToExtent2D(_drawImage.imageExtent), _swapchain.extent);

	// set swapchain image layout to Attachment Optimal so we can draw it
	TransitionImage(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	//draw imgui into the swapchain image
	DrawImgui(cmd, image.imageView);

	// set swapchain image layout to Present so we can draw it
	TransitionImage(cmd, image.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK_abort(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdinfo = CommandBufferSubmitInfo(cmd);

	VkSemaphoreSubmitInfo waitInfo = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frame.swapchainSemaphore);
	VkSemaphoreSubmitInfo signalInfo = SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, image.renderSemaphore);

	VkSubmitInfo2 submit = SubmitInfo(&cmdinfo, &signalInfo, &waitInfo);

	//submit command buffer to the queue and execute it.
	// renderFence will now block until the graphic commands finish execution
	VK_CHECK_abort(vkQueueSubmit2(_graphicsQueue, 1, &submit, frame.renderFence));

	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &image.renderSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &_swapchain.swapchain,
		.pImageIndices = &swapchainImageIndex,
	};
	VK_CHECK_abort(vkQueuePresentKHR(_graphicsQueue, &presentInfo));
}

void Game::DrawBackground(VkCommandBuffer cmd) {
	// bind the gradient drawing compute pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipeline);

	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);

	ComputePushConstants pc;
	pc.data1 = glm::vec4(1, 0, 0, 1);
	pc.data2 = glm::vec4(0, 0, 1, 1);

	vkCmdPushConstants(cmd, _gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &pc);
	// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	vkCmdDispatch(cmd, (uint32_t)std::ceil(_drawImage.imageExtent.width / 16.0), (uint32_t)std::ceil(_drawImage.imageExtent.height / 16.0), 1);
}

void Game::DrawGeometry(VkCommandBuffer cmd) {
	//begin a render pass  connected to our draw image
	VkRenderingAttachmentInfo colorAttachment = RenderingAttachmentInfo(_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VkRenderingInfo renderInfo = RenderingInfo(ToExtent2D(_drawImage.imageExtent), &colorAttachment, nullptr);
	vkCmdBeginRendering(cmd, &renderInfo);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _trianglePipeline);

	//set dynamic viewport and scissor
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = _drawImage.imageExtent.width;
	viewport.height = _drawImage.imageExtent.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = _drawImage.imageExtent.width;
	scissor.extent.height = _drawImage.imageExtent.height;

	vkCmdSetScissor(cmd, 0, 1, &scissor);

	static int counter{0};
	counter++;
	auto model = glm::rotate(glm::scale(glm::mat4(1), glm::vec3(0.5)), counter * 0.1f, glm::vec3(0, 0, 1));
	auto view = glm::lookAtRH(glm::vec3(2),glm::vec3(0),glm::vec3(0,0,1));
	auto proj = glm::infinitePerspectiveRH_NO(glm::radians(90.0f), kScreenWidth / (float) kScreenHeight, 0.1f);
	VertexPushConstants pc{};
	pc.mvp = proj * view * model;
	vkCmdPushConstants(cmd, _trianglePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VertexPushConstants), &pc);

	//launch a draw command to draw 3 vertices
	vkCmdDraw(cmd, 3, 1, 0, 0);

	vkCmdEndRendering(cmd);
}

void Game::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) {
	VK_CHECK_abort(vkResetFences(_device, 1, &_immediateFence));
	VK_CHECK_abort(vkResetCommandBuffer(_immediateCommandBuffer, 0));

	VkCommandBuffer cmd = _immediateCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK_abort(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	function(cmd);

	VK_CHECK_abort(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdinfo = CommandBufferSubmitInfo(cmd);
	VkSubmitInfo2 submit = SubmitInfo(&cmdinfo, nullptr, nullptr);

	// submit command buffer to the queue and execute it.
	//  renderFence will now block until the graphic commands finish execution
	VK_CHECK_abort(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immediateFence));

	VK_CHECK_abort(vkWaitForFences(_device, 1, &_immediateFence, true, 9999999999));
}

void Game::InitImgui() {
	// 1: create descriptor pool for IMGUI
	//  the size of the pool is very oversize, but it's copied from imgui demo
	//  itself.
	VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	VK_CHECK_abort(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool));

	// 2: initialize imgui library

	// this initializes the core structures of imgui
	ImGui::CreateContext();

	// this initializes imgui for SDL
	ImGui_ImplSDL3_InitForVulkan(_window);

	// this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = _instance;
	init_info.PhysicalDevice = _device.physical_device;
	init_info.Device = _device;
	init_info.Queue = _graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.UseDynamicRendering = true;

	//dynamic rendering parameters for imgui to use
	init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &_swapchain.image_format;

	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info);

	ImGui_ImplVulkan_CreateFontsTexture();

	// add the destroy the imgui created structures
	_mainDeletionQueue.PushFunction([=]() {
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(_device, imguiPool, nullptr);
	});
}

void Game::DrawImgui(VkCommandBuffer cmd, VkImageView targetImageView) {
	VkRenderingAttachmentInfo colorAttachment = RenderingAttachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = RenderingInfo(_swapchain.extent, &colorAttachment, nullptr);
	vkCmdBeginRendering(cmd, &renderInfo);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
	vkCmdEndRendering(cmd);
}

Game::FrameData& Game::GetCurrentFrame() {
	return _frames[_frameNumber % FRAME_OVERLAP];
}