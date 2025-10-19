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
	// Vulkan Device Select
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
	// Vulkan Device Build
	auto deviceBuildResult = vkb::DeviceBuilder{ selectedDevice }.build();
	if (!deviceBuildResult) {
		std::cerr << "vkb::DeviceBuilder failed: " << selectResult.error().message() << "\n";
		std::exit(EXIT_FAILURE);
	}
	_device = deviceBuildResult.value();
	// Swapchain
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
	_swapchainImages = _swapchain.get_images().value();
	_swapchainImageViews = _swapchain.get_image_views().value();
}

Game::~Game() {
	for (const auto& view : _swapchainImageViews) {
		vkDestroyImageView(_device, view, _swapchain.allocation_callbacks);
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
	}
}