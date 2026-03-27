#pragma once

#include <Geode/Geode.hpp>
#include "../vkbind.h"
#include "../VulkanRenderer.hpp"

// so close to inheriting CCObject rn
class VKPipeline
{
    protected:
        VkPipeline pipeline = nullptr;
        VkPipelineLayout layout = nullptr;

        static std::pair<VkBlendFactor, VkBlendFactor> ccBlendToVKBlend(cocos2d::ccBlendFunc blendFunc);

        template <typename T>
        static VKPipeline* create(cocos2d::ccBlendFunc blendFunc, VkPrimitiveTopology top);

        static VkPipelineInputAssemblyStateCreateInfo _VkPipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology top);
        static VkPipelineColorBlendStateCreateInfo _VkPipelineColorBlendStateCreateInfo(const VkPipelineColorBlendAttachmentState* cb);
        static VkPipelineColorBlendAttachmentState _VkPipelineColorBlendAttachmentState(cocos2d::ccBlendFunc func);
        static VkPipelineLayoutCreateInfo _VkPipelineLayoutCreateInfo(const VkPushConstantRange* pcRange);
        static VkPipelineRasterizationStateCreateInfo _VkPipelineRasterizationStateCreateInfo();
        static VkPipelineDepthStencilStateCreateInfo _VkPipelineDepthStencilStateCreateInfo();
        static VkPipelineMultisampleStateCreateInfo _VkPipelineMultisampleStateCreateInfo();
        static VkPipelineViewportStateCreateInfo _VkPipelineViewportStateCreateInfo();
        static VkPipelineDynamicStateCreateInfo _VkPipelineDynamicStateCreateInfo();
        static VkPushConstantRange _VkPushConstantRange();
        
        template <typename T>
        static VkPipelineVertexInputStateCreateInfo _VkPipelineVertexInputStateCreateInfo();
        template <>
        VkPipelineVertexInputStateCreateInfo _VkPipelineVertexInputStateCreateInfo<cocos2d::ccV3F_C4B_T2F>();
        template <>
        VkPipelineVertexInputStateCreateInfo _VkPipelineVertexInputStateCreateInfo<cocos2d::ccV2F_C4B_T2F>();
        template <>
        VkPipelineVertexInputStateCreateInfo _VkPipelineVertexInputStateCreateInfo<cocos2d::ccV3F_C4B_T2F_Quad>();
        template <>
        VkPipelineVertexInputStateCreateInfo _VkPipelineVertexInputStateCreateInfo<VkSpriteVertex>();

    public:
        template <typename T>
        static VKPipeline* get(cocos2d::ccBlendFunc blendFunc, VkPrimitiveTopology top = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        void bind();
        void bindTexture(cocos2d::CCTexture2D* texture);

        VkPipeline getPipeline() { return pipeline; }
        VkPipelineLayout getLayout() { return layout; }
};