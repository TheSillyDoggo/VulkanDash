#include "VKPipeline.hpp"
#include "VKShader.hpp"
#include "../Hooks/VKTexture2D.hpp"

using namespace geode::prelude;

void VKPipeline::bind()
{
    static VKPipeline* lastBinded = nullptr;

    if (lastBinded != this)
    {
        lastBinded = this;
        vkCmdBindPipeline(cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            getPipeline());
    }
}

void VKPipeline::bindTexture(CCTexture2D* texture)
{
    static CCTexture2D* lastBindedTexture = nullptr;

    if (lastBindedTexture != texture)
    {
        lastBindedTexture = texture;

        VkDescriptorSet desc = static_cast<VKTexture2D*>(texture)->getDescriptor();
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            getLayout(),
            0,
            1,
            &desc,
            0,
            nullptr
        );
    }
}

template <typename T>
VKPipeline* VKPipeline::get(ccBlendFunc func, VkPrimitiveTopology top)
{
    return nullptr;
}

template <>
VKPipeline* VKPipeline::get<VkSpriteVertex>(ccBlendFunc func, VkPrimitiveTopology top)
{
    static std::map<std::pair<GLenum, GLenum>, VKPipeline*> pipelines = {};
    std::pair<GLenum, GLenum> fnc = { func.src, func.dst };

    if (!pipelines.contains(fnc))
        pipelines[fnc] = create<VkSpriteVertex>(func, top);

    return pipelines[fnc];
}

template <>
VKPipeline* VKPipeline::get<ccV3F_C4B_T2F>(ccBlendFunc func, VkPrimitiveTopology top)
{
    static std::map<std::pair<GLenum, GLenum>, VKPipeline*> pipelines = {};
    std::pair<GLenum, GLenum> fnc = { func.src, func.dst };

    if (!pipelines.contains(fnc))
        pipelines[fnc] = create<ccV3F_C4B_T2F>(func, top);

    return pipelines[fnc];
}

template <>
VKPipeline* VKPipeline::get<ccV2F_C4B_T2F>(ccBlendFunc func, VkPrimitiveTopology top)
{
    static std::map<std::pair<GLenum, GLenum>, VKPipeline*> pipelines = {};
    std::pair<GLenum, GLenum> fnc = { func.src, func.dst };

    if (!pipelines.contains(fnc))
        pipelines[fnc] = create<ccV2F_C4B_T2F>(func, top);

    return pipelines[fnc];
}

template <>
VKPipeline* VKPipeline::get<ccV3F_C4B_T2F_Quad>(ccBlendFunc func, VkPrimitiveTopology top)
{
    static std::map<std::pair<GLenum, GLenum>, VKPipeline*> pipelines = {};
    std::pair<GLenum, GLenum> fnc = { func.src, func.dst };

    if (!pipelines.contains(fnc))
        pipelines[fnc] = create<ccV3F_C4B_T2F_Quad>(func, top);

    return pipelines[fnc];
}

std::pair<VkBlendFactor, VkBlendFactor> VKPipeline::ccBlendToVKBlend(ccBlendFunc func)
{
    VkBlendFactor src;
    VkBlendFactor dst;

    switch (func.src)
    {
        case GL_ONE:
            src = VK_BLEND_FACTOR_ONE; break;
        case GL_SRC_COLOR:
            src = VK_BLEND_FACTOR_SRC_COLOR; break;
        case GL_ONE_MINUS_SRC_COLOR:
            src = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR; break;
        case GL_SRC_ALPHA:
            src = VK_BLEND_FACTOR_SRC_ALPHA; break;
        case GL_ONE_MINUS_SRC_ALPHA:
            src = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; break;
        case GL_DST_ALPHA:
            src = VK_BLEND_FACTOR_DST_ALPHA; break;
        case GL_ONE_MINUS_DST_ALPHA:
            src = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA; break;
        case GL_DST_COLOR:
            src = VK_BLEND_FACTOR_DST_COLOR; break;
        case GL_ONE_MINUS_DST_COLOR:
            src = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR; break;
        case GL_SRC_ALPHA_SATURATE:
            src = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE; break;
        default:
            log::error("unknown src: {}", (int)func.src);
            break;
    }

    switch (func.dst)
    {
        case GL_ONE:
            dst = VK_BLEND_FACTOR_ONE; break;
        case GL_SRC_COLOR:
            dst = VK_BLEND_FACTOR_SRC_COLOR; break;
        case GL_ONE_MINUS_SRC_COLOR:
            dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR; break;
        case GL_SRC_ALPHA:
            dst = VK_BLEND_FACTOR_SRC_ALPHA; break;
        case GL_ONE_MINUS_SRC_ALPHA:
            dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; break;
        case GL_DST_ALPHA:
            dst = VK_BLEND_FACTOR_DST_ALPHA; break;
        case GL_ONE_MINUS_DST_ALPHA:
            dst = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA; break;
        case GL_DST_COLOR:
            dst = VK_BLEND_FACTOR_DST_COLOR; break;
        case GL_ONE_MINUS_DST_COLOR:
            dst = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR; break;
        case GL_SRC_ALPHA_SATURATE:
            dst = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE; break;
        default:
            log::error("unknown dst: {}", (int)func.dst);
            break;
    }

    return {src, dst};
}

template <typename T>
VKPipeline* VKPipeline::create(ccBlendFunc func, VkPrimitiveTopology top)
{
    auto vertSpv = VKShader::create("vert.spv"_spr, "vert.vert"_spr);
    auto fragSpv = VKShader::create("frag.spv"_spr, "frag.frag"_spr);

    VKPipeline* pl = new VKPipeline();

    VkShaderModule vertShader, fragShader;
    VkShaderModuleCreateInfo smci{};
    smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    smci.codeSize = vertSpv.size() * sizeof(uint32_t);
    smci.pCode = vertSpv.data();
    vkCreateShaderModule(device, &smci, nullptr, &vertShader);

    smci.codeSize = fragSpv.size() * sizeof(uint32_t);
    smci.pCode = fragSpv.data();
    vkCreateShaderModule(device, &smci, nullptr, &fragShader);

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertShader;
    stages[0].pName = "main";

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragShader;
    stages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo visci = _VkPipelineVertexInputStateCreateInfo<T>();
    VkPipelineInputAssemblyStateCreateInfo iasci = _VkPipelineInputAssemblyStateCreateInfo(top);
    VkPipelineViewportStateCreateInfo vpsci = _VkPipelineViewportStateCreateInfo();
    VkPipelineRasterizationStateCreateInfo rsci = _VkPipelineRasterizationStateCreateInfo();
    VkPipelineMultisampleStateCreateInfo msci = _VkPipelineMultisampleStateCreateInfo();
    VkPipelineColorBlendAttachmentState cb = _VkPipelineColorBlendAttachmentState(func);
    VkPipelineColorBlendStateCreateInfo cbci = _VkPipelineColorBlendStateCreateInfo(&cb);
    VkPipelineDepthStencilStateCreateInfo dss = _VkPipelineDepthStencilStateCreateInfo();

    VkPushConstantRange pcRange = _VkPushConstantRange();
    VkPipelineLayoutCreateInfo plci = _VkPipelineLayoutCreateInfo(&pcRange);
    vkCreatePipelineLayout(device, &plci, nullptr, &pl->layout);

    VkPipelineDynamicStateCreateInfo dyst = _VkPipelineDynamicStateCreateInfo();

    VkGraphicsPipelineCreateInfo gpci{};
    gpci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gpci.stageCount = 2;
    gpci.pStages = stages;
    gpci.pVertexInputState = &visci;
    gpci.pInputAssemblyState = &iasci;
    gpci.pViewportState = &vpsci;
    gpci.pRasterizationState = &rsci;
    gpci.pMultisampleState = &msci;
    gpci.pColorBlendState = &cbci;
    gpci.layout = pl->layout;
    gpci.renderPass = renderPass;
    gpci.pDynamicState = &dyst;
    gpci.pDepthStencilState = &dss;
    gpci.subpass = 0;

    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gpci, nullptr, &pl->pipeline);

    vkDestroyShaderModule(device, vertShader, nullptr);
    vkDestroyShaderModule(device, fragShader, nullptr);
    return pl;
}

VkPipelineDynamicStateCreateInfo VKPipeline::_VkPipelineDynamicStateCreateInfo()
{
    static VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dyst{};
    dyst.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyst.dynamicStateCount = 1;
    dyst.pDynamicStates = dynamicStates;

    return std::move(dyst);
}

VkPipelineColorBlendAttachmentState VKPipeline::_VkPipelineColorBlendAttachmentState(cocos2d::ccBlendFunc func)
{
    VkPipelineColorBlendAttachmentState cb{};
    cb.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    cb.blendEnable = VK_TRUE;

    auto blend = ccBlendToVKBlend(func);

    cb.srcColorBlendFactor = blend.first;
    cb.dstColorBlendFactor = blend.second;
    cb.colorBlendOp = VK_BLEND_OP_ADD;

    cb.srcAlphaBlendFactor = blend.first;
    cb.dstAlphaBlendFactor = blend.second;
    cb.alphaBlendOp = VK_BLEND_OP_ADD;

    cb.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;

    return std::move(cb);
}

VkPipelineRasterizationStateCreateInfo VKPipeline::_VkPipelineRasterizationStateCreateInfo()
{
    VkPipelineRasterizationStateCreateInfo rsci{};
    rsci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rsci.cullMode = VK_CULL_MODE_NONE;
    rsci.polygonMode = VK_POLYGON_MODE_FILL;
    rsci.lineWidth = 0;
    rsci.frontFace = VK_FRONT_FACE_CLOCKWISE;

    return std::move(rsci);
}

VkPipelineMultisampleStateCreateInfo VKPipeline::_VkPipelineMultisampleStateCreateInfo()
{
    VkPipelineMultisampleStateCreateInfo msci{};
    msci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    return std::move(msci);
}

VkPipelineLayoutCreateInfo VKPipeline::_VkPipelineLayoutCreateInfo(const VkPushConstantRange* pcRange)
{
    VkPipelineLayoutCreateInfo plci{};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges = pcRange;
    plci.setLayoutCount = 1;
    plci.pSetLayouts = &textureLayout;

    return std::move(plci);
}

VkPushConstantRange VKPipeline::_VkPushConstantRange()
{
    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pcRange.offset = 0;
    pcRange.size = sizeof(float) * 16; // 4x4 matrix

    return std::move(pcRange);
}

VkPipelineViewportStateCreateInfo VKPipeline::_VkPipelineViewportStateCreateInfo()
{
    static VkViewport vp{};
    vp.x = 0; vp.y = (float)swapExtent.height;
    vp.width = (float)swapExtent.width;
    vp.height = -(float)swapExtent.height;
    vp.minDepth = 0.0f; vp.maxDepth = 1.0f;

    VkPipelineViewportStateCreateInfo vpsci{};
    vpsci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpsci.viewportCount = 1; vpsci.pViewports = &vp;
    vpsci.scissorCount = 1;

    return std::move(vpsci);
}

VkPipelineInputAssemblyStateCreateInfo VKPipeline::_VkPipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology top)
{
    VkPipelineInputAssemblyStateCreateInfo iasci{};
    iasci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iasci.topology = top;

    return std::move(iasci);
}

VkPipelineColorBlendStateCreateInfo VKPipeline::_VkPipelineColorBlendStateCreateInfo(const VkPipelineColorBlendAttachmentState* cb)
{
    VkPipelineColorBlendStateCreateInfo cbci{};
    cbci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbci.attachmentCount = 1;
    cbci.pAttachments = cb;

    return std::move(cbci);
}

VkPipelineDepthStencilStateCreateInfo VKPipeline::_VkPipelineDepthStencilStateCreateInfo()
{
    VkPipelineDepthStencilStateCreateInfo dss{};
    dss.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dss.depthTestEnable = VK_FALSE;//VK_TRUE;
    dss.depthWriteEnable = VK_TRUE;
    dss.depthCompareOp = VK_COMPARE_OP_LESS;// ;
    dss.depthBoundsTestEnable = VK_FALSE;
    dss.stencilTestEnable = VK_FALSE;

    return std::move(dss);
}


template <typename T>
VkPipelineVertexInputStateCreateInfo VKPipeline::_VkPipelineVertexInputStateCreateInfo()
{
    VkPipelineVertexInputStateCreateInfo visci{};

    return std::move(visci);
}

template <>
VkPipelineVertexInputStateCreateInfo VKPipeline::_VkPipelineVertexInputStateCreateInfo<cocos2d::ccV3F_C4B_T2F>()
{
    static VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(ccV3F_C4B_T2F);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    static VkVertexInputAttributeDescription attrs[3]{};
    attrs[0].binding = 0;
    attrs[0].location = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = offsetof(ccV3F_C4B_T2F, vertices);

    attrs[1].binding = 0;
    attrs[1].location = 1;
    attrs[1].format = VK_FORMAT_R8G8B8A8_UNORM;
    attrs[1].offset = offsetof(ccV3F_C4B_T2F, colors);

    attrs[2].binding = 0;
    attrs[2].location = 2;
    attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[2].offset = offsetof(ccV3F_C4B_T2F, texCoords);

    VkPipelineVertexInputStateCreateInfo visci{};
    visci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    visci.vertexBindingDescriptionCount = 1;
    visci.pVertexBindingDescriptions = &binding;
    visci.vertexAttributeDescriptionCount = 3;
    visci.pVertexAttributeDescriptions = attrs;

    return std::move(visci);
}

template <>
VkPipelineVertexInputStateCreateInfo VKPipeline::_VkPipelineVertexInputStateCreateInfo<cocos2d::ccV2F_C4B_T2F>()
{
    static VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(ccV2F_C4B_T2F);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    static VkVertexInputAttributeDescription attrs[3]{};
    attrs[0].binding = 0;
    attrs[0].location = 0;
    attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[0].offset = offsetof(ccV2F_C4B_T2F, vertices);

    attrs[1].binding = 0;
    attrs[1].location = 1;
    attrs[1].format = VK_FORMAT_R8G8B8A8_UNORM;
    attrs[1].offset = offsetof(ccV2F_C4B_T2F, colors);

    attrs[2].binding = 0;
    attrs[2].location = 2;
    attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[2].offset = offsetof(ccV2F_C4B_T2F, texCoords);

    VkPipelineVertexInputStateCreateInfo visci{};
    visci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    visci.vertexBindingDescriptionCount = 1;
    visci.pVertexBindingDescriptions = &binding;
    visci.vertexAttributeDescriptionCount = 3;
    visci.pVertexAttributeDescriptions = attrs;

    return std::move(visci);
}

template <>
VkPipelineVertexInputStateCreateInfo VKPipeline::_VkPipelineVertexInputStateCreateInfo<cocos2d::ccV3F_C4B_T2F_Quad>()
{
    static VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(ccV3F_C4B_T2F);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    static VkVertexInputAttributeDescription attrs[3]{};
    attrs[0].binding = 0;
    attrs[0].location = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = offsetof(ccV3F_C4B_T2F, vertices);

    attrs[1].binding = 0;
    attrs[1].location = 1;
    attrs[1].format = VK_FORMAT_R8G8B8A8_UNORM;
    attrs[1].offset = offsetof(ccV3F_C4B_T2F, colors);

    attrs[2].binding = 0;
    attrs[2].location = 2;
    attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[2].offset = offsetof(ccV3F_C4B_T2F, texCoords);

    VkPipelineVertexInputStateCreateInfo visci{};
    visci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    visci.vertexBindingDescriptionCount = 1;
    visci.pVertexBindingDescriptions = &binding;
    visci.vertexAttributeDescriptionCount = 3;
    visci.pVertexAttributeDescriptions = attrs;

    return std::move(visci);
}

template <>
VkPipelineVertexInputStateCreateInfo VKPipeline::_VkPipelineVertexInputStateCreateInfo<VkSpriteVertex>()
{
    static VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(VkSpriteVertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    static VkVertexInputAttributeDescription attrs[3]{};
    attrs[0].binding = 0;
    attrs[0].location = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = offsetof(VkSpriteVertex, pos);

    attrs[1].binding = 0;
    attrs[1].location = 1;
    attrs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[1].offset = offsetof(VkSpriteVertex, color);

    attrs[2].binding = 0;
    attrs[2].location = 2;
    attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[2].offset = offsetof(VkSpriteVertex, uv);

    VkPipelineVertexInputStateCreateInfo visci{};
    visci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    visci.vertexBindingDescriptionCount = 1;
    visci.pVertexBindingDescriptions = &binding;
    visci.vertexAttributeDescriptionCount = 3;
    visci.pVertexAttributeDescriptions = attrs;

    return std::move(visci);
}