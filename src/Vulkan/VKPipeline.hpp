#pragma once

#include <Geode/Geode.hpp>
#include "../vkbind.h"
#include "../VulkanRenderer.hpp"

enum class VKPipelineType
{
    TriangleList = 0
};

class VKPipeline
{
    protected:
        VkPipeline pipeline = nullptr;
        VkPipelineLayout layout = nullptr;

        static std::pair<VkBlendFactor, VkBlendFactor> ccBlendToVKBlend(cocos2d::ccBlendFunc blendFunc);

        template <typename T>
        static VKPipeline* create(cocos2d::ccBlendFunc blendFunc);

        template<>
        VKPipeline* create<VkSpriteVertex>(cocos2d::ccBlendFunc blendFunc);

        template<>
        VKPipeline* create<cocos2d::ccV3F_C4B_T2F>(cocos2d::ccBlendFunc blendFunc);

    public:
        template <typename T>
        static VKPipeline* get(cocos2d::ccBlendFunc blendFunc);

        VkPipeline getPipeline() { return pipeline; }
        VkPipelineLayout getLayout() { return layout; }
};