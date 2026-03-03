#include "VKTexture2D.hpp"

std::unordered_map<CCTexture2D*, VKTexture2D::VKData> data = {};

static uint32_t findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags props)
{
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
    {
        if ((typeBits & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }

    return 0;
}

static VkCommandBuffer beginCmd()
{
    VkCommandBufferAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc.commandPool = cmdPool;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device, &alloc, &cmd);

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin);
    return cmd;
}

static void endCmd(VkCommandBuffer cmd)
{
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    vkQueueSubmit(graphicsQueue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, cmdPool, 1, &cmd);
}

bool VKTexture2D::initWithData(const void* data, CCTexture2DPixelFormat pixelFormat, unsigned int pixelsWide, unsigned int pixelsHigh, const CCSize& contentSize)
{
    // stops render textures crashing
    // really bad
    if (pixelsWide == 1920 && pixelsHigh == 1080)
        return CCTexture2D::initWithData(data, pixelFormat, pixelsWide, pixelsHigh, contentSize);

    if (!data) return false;

    if (!CCTexture2D::initWithData(data, pixelFormat, pixelsWide, pixelsHigh, contentSize))
        return false;

    VKData vk{};

    VkFormat vkFormat = ccFormatToVKFormat(pixelFormat);
    if (vkFormat == VK_FORMAT_UNDEFINED) return false;

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(pixelsWide) * pixelsHigh * bitsPerPixelForFormat(pixelFormat) / 8;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { pixelsWide, pixelsHigh, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = vkFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(vma, &imageInfo, &allocInfo, &vk.image, &vk.memory, nullptr) != VK_SUCCESS)
        return false;

    VkBuffer stagingBuffer;
    VmaAllocation stagingAlloc;

    VkBufferCreateInfo bufInfo{};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size = imageSize;
    bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo bufAllocInfo{};
    bufAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

    if (vmaCreateBuffer(vma, &bufInfo, &bufAllocInfo, &stagingBuffer, &stagingAlloc, nullptr) != VK_SUCCESS)
        return false;

    void* mapped = nullptr;
    vmaMapMemory(vma, stagingAlloc, &mapped);
    memcpy(mapped, data, imageSize);
    vmaUnmapMemory(vma, stagingAlloc);

    VkCommandBuffer cmd = beginCmd();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.image = vk.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier
    );

    VkBufferImageCopy copyRegion{};
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageExtent = { pixelsWide, pixelsHigh, 1 };

    vkCmdCopyBufferToImage(
        cmd,
        stagingBuffer,
        vk.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copyRegion
    );

    VkImageMemoryBarrier toShader{};
    toShader.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toShader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toShader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toShader.image = vk.image;
    toShader.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toShader.subresourceRange.levelCount = 1;
    toShader.subresourceRange.layerCount = 1;
    toShader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toShader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &toShader
    );

    endCmd(cmd);

    vmaDestroyBuffer(vma, stagingBuffer, stagingAlloc);

    VkImageViewCreateInfo view{};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.image = vk.image;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = vkFormat;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.layerCount = 1;

    vkCreateImageView(device, &view, nullptr, &vk.imageView);

    VkSamplerCreateInfo samp{};
    samp.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samp.magFilter = VK_FILTER_LINEAR;
    samp.minFilter = VK_FILTER_LINEAR;
    samp.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samp.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samp.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    vkCreateSampler(device, &samp, nullptr, &vk.sampler);

    VkDescriptorSetAllocateInfo dsAlloc{};
    dsAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsAlloc.descriptorPool = descriptorPool;
    dsAlloc.descriptorSetCount = 1;
    dsAlloc.pSetLayouts = &textureLayout;

    vkAllocateDescriptorSets(device, &dsAlloc, &vk.descriptorSet);

    VkDescriptorImageInfo imgInfo{};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imgInfo.imageView = vk.imageView;
    imgInfo.sampler = vk.sampler;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = vk.descriptorSet;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imgInfo;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    ::data[this] = vk;
    return true;
}

void VKTexture2D::destructor()
{
    log::error("DEALLOCATING!!");
    // todo: delete the vulkan shit so it doesnt memory leak

    CCTexture2D::~CCTexture2D();
}

VkImageView VKTexture2D::getImageView()
{
    return data[this].imageView;
}

VkSampler VKTexture2D::getSampler()
{
    return data[this].sampler;
}

VkDescriptorSet VKTexture2D::getDescriptor()
{
    return data[this].descriptorSet;
}

VkFormat VKTexture2D::ccFormatToVKFormat(CCTexture2DPixelFormat format)
{
    switch (format)
    {
        case kCCTexture2DPixelFormat_RGBA8888:
            return VK_FORMAT_R8G8B8A8_UNORM;

        case kCCTexture2DPixelFormat_RGB888:
            return VK_FORMAT_R8G8B8_UNORM;

        case kCCTexture2DPixelFormat_RGB565:
            return VK_FORMAT_R5G6B5_UNORM_PACK16;

        case kCCTexture2DPixelFormat_RGBA4444:
            return VK_FORMAT_R4G4B4A4_UNORM_PACK16;

        case kCCTexture2DPixelFormat_A8:
            return VK_FORMAT_R8_UNORM;

        default:
            log::error("Unsupported pixel format");
            return VK_FORMAT_UNDEFINED;
    }
}