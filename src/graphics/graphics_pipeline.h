#pragma once
#include <vector>
#include <vulkan/vulkan.h>

class PipelineBuilder {
public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout _pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    VkPipelineRenderingCreateInfo _renderInfo;
    VkFormat _colorAttachmentformat;

    PipelineBuilder();
    void Reset();

    void SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void SetColorAttachmentFormat(VkFormat format);
    void SetDepthFormat(VkFormat format);

    VkPipeline BuildPipeline(VkDevice device);
};