#pragma once

#include "GenericRenderer.hpp"
#define VK_USE_PLATFORM_WIN32_KHR
#include "vkbind.h"
#include "vk_mem_alloc.h"
#include <Geode/Geode.hpp>

class VKCocosOwnedMemory;

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
extern unsigned int drawOrder;
extern unsigned int drawCalls;
extern unsigned int bindCalls;
extern unsigned int bindTextureCalls;

class VulkanRenderer : public GenericRenderer
{
    public:
        virtual bool init();
        virtual bool begin();
        virtual void present();
        virtual void end();
};

struct VkSpriteVertex {
    float pos[3] = {0,0,0};
    float color[4] = {0,0,0,0};
    float uv[2] = {0,0};
};

struct DrawCommand
{
    kmMat4 mat;
    cocos2d::ccBlendFunc func;
    cocos2d::CCTexture2D* texture;
    VKCocosOwnedMemory* memory;
    bool scissorEnabled;
    cocos2d::CCRect scissorRect;
};

kmMat4 getNodeToWorldTransform(cocos2d::CCNode* node);
void updateScissor();
float getFakeZ();
VulkanRenderer* getRenderer();
void handleDrawCommand(cocos2d::CCSprite* sprite, VKCocosOwnedMemory* memory);

#define INCREMENT_DRAW_CALLS(num) \
drawCalls += num;
#define INCREMENT_BIND_CALLS(num) \
bindCalls += num;
#define INCREMENT_BIND_TEX_CALLS(num) \
bindTextureCalls += num;