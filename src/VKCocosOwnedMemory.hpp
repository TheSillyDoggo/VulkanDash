#pragma once

#include <Geode/Geode.hpp>
#include "vkbind.h"
#include "vk_mem_alloc.h"

class VKCocosOwnedMemory : public cocos2d::CCObject
{
    protected:
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation alloc = VK_NULL_HANDLE;
        void* mapped = nullptr;
        VkDeviceSize size = 0;
        VkBufferUsageFlags usage = VK_SHARING_MODE_EXCLUSIVE;

        ~VKCocosOwnedMemory();

        bool init(VkDeviceSize newSize, VkBufferUsageFlags usage);

    public:
        static VKCocosOwnedMemory* create(VkDeviceSize size, VkBufferUsageFlags usage = VK_SHARING_MODE_EXCLUSIVE);

        VkBuffer getBuffer() const { return buffer; }
        void* getMapped() const { return mapped; }
        VkDeviceSize getSize() const { return size; }

        void resize(VkDeviceSize newSize);
        void upload(const void* src, VkDeviceSize size, VkDeviceSize offset = 0);
};