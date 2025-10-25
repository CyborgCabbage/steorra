add_executable (steorra "steorra.cpp" "game.cpp" "game.h"  "graphics/graphics_data.h" "graphics/graphics_command.cpp" "graphics/graphics_command.h" "graphics/graphics_memory.h" "graphics/graphics_memory.cpp" "graphics/graphics_shaders.h" "graphics/graphics_shaders.cpp" "graphics/graphics_errors.h")

target_include_directories(steorra PRIVATE "")
target_include_directories(steorra PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(steorra PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(steorra PRIVATE glm::glm)
# Or use the header-only version
#target_link_libraries(main PRIVATE glm::glm-header-only)
target_link_libraries(steorra PRIVATE SDL3::SDL3)
target_link_libraries(steorra PRIVATE vk-bootstrap::vk-bootstrap)
target_link_libraries(steorra PRIVATE imgui::imgui)

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/assets/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/assets/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/assets/shaders/*.comp"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/assets/shaders/${FILE_NAME}.spv")
  message(STATUS ${GLSL})
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
)

add_custom_target(CopyAssets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets
)
add_dependencies(steorra CopyAssets)