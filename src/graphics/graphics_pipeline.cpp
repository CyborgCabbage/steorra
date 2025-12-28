#include "graphics_pipeline.h"
#include <array>
#include "graphics/graphics_data.h"

PipelineBuilder::PipelineBuilder() {
	Reset();
}


void PipelineBuilder::Reset() {
    _inputAssembly = { 
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    _rasterizer = { 
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f, // Even though we are not rendering lines, Vulkan complains if this is not 1.0
    };

    // .blendEnabled is inited to 0 and therefore false
    _colorBlendAttachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    _multisampling = { 
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        // no alpha to coverage either
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    _pipelineLayout = {};

    _depthStencil = { 
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    };

    _renderInfo = { 
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO 
    };

    _shaderStages.clear();
}

void PipelineBuilder::SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader) {
    _shaderStages.clear();
    _shaderStages.push_back(PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
    _shaderStages.push_back(PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
}

void PipelineBuilder::SetColorAttachmentFormat(VkFormat format) {
    _colorAttachmentformat = format;
    // connect the format to the renderInfo  structure
    _renderInfo.colorAttachmentCount = 1;
    _renderInfo.pColorAttachmentFormats = &_colorAttachmentformat;
}

void PipelineBuilder::SetDepthFormat(VkFormat format) {
    _renderInfo.depthAttachmentFormat = format;
}

void PipelineBuilder::SetDepthTest(bool test, bool write, VkCompareOp op) {
    _depthStencil.depthTestEnable = test;
    _depthStencil.depthWriteEnable = write;
    _depthStencil.depthCompareOp = op;
    _depthStencil.minDepthBounds = 0.0f;
    _depthStencil.maxDepthBounds = 1.0f;
}

VkPipeline PipelineBuilder::BuildPipeline(VkDevice device) {
    // Don't need to provide ptrs because we are using dynamic viewport and scissor state
    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    // 
    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &_colorBlendAttachment,
    };

    // completely clear VertexInputStateCreateInfo, as we have no need for it
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = { 
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO 
    };

    std::array<VkDynamicState, 2> state{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = state.data()
    };

    VkGraphicsPipelineCreateInfo pipelineInfo = { 
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &_renderInfo,
        .stageCount = (uint32_t)_shaderStages.size(),
        .pStages = _shaderStages.data(),
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &_inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &_rasterizer,
        .pMultisampleState = &_multisampling,
        .pDepthStencilState = &_depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicInfo,
        .layout = _pipelineLayout,
    };
    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    } else {
        return newPipeline;
    }
}
