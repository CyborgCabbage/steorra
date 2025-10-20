#pragma once
#include <vulkan/vulkan.h>
#include <iostream>

#define VK_CHECK_abort(x)\
	do {\
		VkResult err = x;\
		if (err) {\
			std::cout <<"Detected Vulkan error: " << err << std::endl;\
			abort();\
		}\
	} while (0)