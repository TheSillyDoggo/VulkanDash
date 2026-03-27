#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/CCTexture2D.hpp>
#include "../vkbind.h"
#include "../VulkanRenderer.hpp"

using namespace geode::prelude;

class $modify (VKTexture2D, CCTexture2D)
{
    struct VKData
    {
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation memory = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };

    // hooks
    bool initWithData(const void* data, CCTexture2DPixelFormat pixelFormat, unsigned int pixelsWide, unsigned int pixelsHigh, const CCSize& contentSize);
    void destructor();
    void setTexParameters(ccTexParams* texParams);

    VkImageView getImageView();
    VkSampler getSampler();
    VkDescriptorSet getDescriptor();

    static VkFormat ccFormatToVKFormat(CCTexture2DPixelFormat format);
};