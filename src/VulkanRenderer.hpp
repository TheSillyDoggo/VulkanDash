#pragma once

#include "GenericRenderer.hpp"
#define VK_USE_PLATFORM_WIN32_KHR
#include "vkbind.h"
#include "vk_mem_alloc.h"

namespace cocos2d
{
    class CCNode;
    class CCRect;
};

extern VkDevice device;
extern VkPhysicalDevice physicalDevice;
extern VkQueue graphicsQueue;
extern VkCommandPool cmdPool;
extern VkDescriptorPool descriptorPool;
extern VkDescriptorSetLayout textureLayout;
extern VkRenderPass renderPass;
extern VkExtent2D swapExtent;
extern VkCommandBuffer cmd;
extern VmaAllocator vma;

class VulkanRenderer : public GenericRenderer
{
    public:
        virtual bool init();
        virtual bool begin();
        virtual void present();
};

struct VkSpriteVertex {
    float pos[3] = {0,0,0};
    float color[4] = {0,0,0,0};
    float uv[2] = {0,0};
};

kmMat4 getNodeToWorldTransform(cocos2d::CCNode* node);
void updateScissor();