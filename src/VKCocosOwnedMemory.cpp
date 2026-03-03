#include "VKCocosOwnedMemory.hpp"
#include "VulkanRenderer.hpp"

using namespace geode::prelude;

VKCocosOwnedMemory* VKCocosOwnedMemory::create(VkDeviceSize size, VkBufferUsageFlags usage)
{
    auto pRet = new VKCocosOwnedMemory();

    if (pRet && pRet->init(size, usage))
    {
        pRet->autorelease();
        return pRet;
    }

    CC_SAFE_DELETE(pRet);
    return nullptr;
}

bool VKCocosOwnedMemory::init(VkDeviceSize newSize, VkBufferUsageFlags usage)
{
    newSize = std::max<VkDeviceSize>(8, newSize);

    size = newSize;
    this->usage = usage;

    VkBufferCreateInfo buf{};
    buf.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf.size  = newSize;
    buf.usage = usage |
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buf.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
        VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo outInfo{};
    if (vmaCreateBuffer(
            vma,
            &buf,
            &allocInfo,
            &buffer,
            &alloc,
            &outInfo
        ) != VK_SUCCESS)
    {
        log::error("vmaCreateBuffer failed (size={})", newSize);
        return false;
    }

    mapped = outInfo.pMappedData;
    return true;
}

VKCocosOwnedMemory::~VKCocosOwnedMemory()
{
    if (buffer)
    {
        vmaDestroyBuffer(vma, buffer, alloc);
        buffer = VK_NULL_HANDLE;
        alloc  = VK_NULL_HANDLE;
        mapped = nullptr;
    }
}

void VKCocosOwnedMemory::resize(VkDeviceSize newSize)
{
    if (newSize <= size)
        return;

    if (buffer)
        vmaDestroyBuffer(vma, buffer, alloc);

    buffer = VK_NULL_HANDLE;
    alloc  = VK_NULL_HANDLE;
    mapped = nullptr;

    init(newSize, usage);
}

void VKCocosOwnedMemory::upload(const void* src, VkDeviceSize dataSize, VkDeviceSize offset)
{
    if (dataSize > this->size)
        log::error("writing {} when size is {}", dataSize, this->size);

    memcpy(static_cast<char*>(mapped) + offset, src, dataSize);
}