add_executable (steorra "steorra.cpp" "game.cpp" "game.h" "graphics/graphics_data.cpp" "graphics/graphics_data.h" "graphics/graphics_command.cpp" "graphics/graphics_command.h" "graphics/graphics_memory.h" "graphics/graphics_memory.cpp")
target_include_directories(steorra PRIVATE "")
target_include_directories(steorra PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(steorra PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(steorra PRIVATE glm::glm)
# Or use the header-only version
#target_link_libraries(main PRIVATE glm::glm-header-only)
target_link_libraries(steorra PRIVATE SDL3::SDL3)
target_link_libraries(steorra PRIVATE vk-bootstrap::vk-bootstrap)
