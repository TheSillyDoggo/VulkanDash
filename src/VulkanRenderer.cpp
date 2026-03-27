#include "VulkanRenderer.hpp"
#include <Geode/Geode.hpp>
#include "Hooks/VKTexture2D.hpp"

using namespace geode::prelude;

VmaAllocator vma;
VkInstance instance;
VkSurfaceKHR surface;
VkPhysicalDevice physicalDevice;
VkDevice device;
VkQueue graphicsQueue;
uint32_t queueIndex;

VkSwapchainKHR swapchain;
VkFormat swapFormat;
VkExtent2D swapExtent;

std::vector<VkImage> swapImages;
std::vector<VkImageView> swapImageViews;
std::vector<VkFramebuffer> framebuffers;

VkRenderPass renderPass;
VkCommandPool cmdPool;
VkCommandBuffer cmd;

VkSemaphore imageAvailable;
VkSemaphore renderFinished;
VkFence inFlightFence;
unsigned int drawOrder = 0;
unsigned int drawCalls = 0;
unsigned int bindCalls = 0;
unsigned int bindTextureCalls = 0;

void CreateInstance()
{
    const char* extensions[] = {
        "VK_KHR_surface",
        "VK_KHR_win32_surface"
    };

    VkInstanceCreateInfo ici{};
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.enabledExtensionCount = 2;
    ici.ppEnabledExtensionNames = extensions;

    vkCreateInstance(&ici, nullptr, &instance);
}

void CreateSurface(HWND hwnd)
{
    VkWin32SurfaceCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    sci.hwnd = hwnd;
    sci.hinstance = GetModuleHandle(nullptr);

    vkCreateWin32SurfaceKHR(instance, &sci, nullptr, &surface);
}

void CreateDevice()
{
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());
    physicalDevice = devices[0];

    uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);
    std::vector<VkQueueFamilyProperties> queues(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queues.data());

    for (uint32_t i = 0; i < queueCount; i++)
    {
        VkBool32 present = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &present);

        if ((queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present)
        {
            queueIndex = i;
            break;
        }
    }

    float priority = 1.0f;

    VkDeviceQueueCreateInfo qci{};
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = queueIndex;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;

    const char* extensions[] = { "VK_KHR_swapchain" };

    VkDeviceCreateInfo dci{};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = 1;
    dci.ppEnabledExtensionNames = extensions;

    vkCreateDevice(physicalDevice, &dci, nullptr, &device);
    vkGetDeviceQueue(device, queueIndex, 0, &graphicsQueue);
}

VkImage depthImage;
VmaAllocation depthAllocation;
VkImageView depthImageView;

void CreateSwapchain(uint32_t width, uint32_t height)
{
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps);

    swapFormat = VK_FORMAT_B8G8R8A8_UNORM;
    swapExtent = { width, height };

    VkSwapchainCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface = surface;
    sci.minImageCount = 2;
    sci.imageFormat = swapFormat;
    sci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sci.imageExtent = swapExtent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sci.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    sci.clipped = VK_TRUE;

    vkCreateSwapchainKHR(device, &sci, nullptr, &swapchain);

    uint32_t count;
    vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr);
    swapImages.resize(count);
    vkGetSwapchainImagesKHR(device, swapchain, &count, swapImages.data());



    VkImageCreateInfo img{};
    img.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img.imageType = VK_IMAGE_TYPE_2D;
    img.format = VK_FORMAT_D32_SFLOAT;
    img.extent = { swapExtent.width, swapExtent.height, 1 };
    img.mipLevels = 1;
    img.arrayLayers = 1;
    img.samples = VK_SAMPLE_COUNT_1_BIT;
    img.tiling = VK_IMAGE_TILING_OPTIMAL;
    img.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    img.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo alloc{};
    alloc.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    vmaCreateImage(vma, &img, &alloc, &depthImage, &depthAllocation, nullptr);

    VkImageViewCreateInfo view{};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.image = depthImage;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = VK_FORMAT_D32_SFLOAT;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view.subresourceRange.baseMipLevel = 0;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.baseArrayLayer = 0;
    view.subresourceRange.layerCount = 1;

    vkCreateImageView(device, &view, nullptr, &depthImageView);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.image = depthImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

void recreateSwapchain(uint32_t width, uint32_t height) {
    vkDeviceWaitIdle(device);

    for(auto fb : framebuffers)
        vkDestroyFramebuffer(device, fb, nullptr);

    for(auto iv : swapImageViews)
        vkDestroyImageView(device, iv, nullptr);

    vkDestroySwapchainKHR(device, swapchain, nullptr);

    vkDestroyImageView(device, depthImageView, nullptr);
    vmaDestroyImage(vma, depthImage, depthAllocation);

    CreateSwapchain(width, height);

    swapImageViews.resize(swapImages.size());
    for(size_t i = 0; i < swapImages.size(); i++) {
        VkImageViewCreateInfo iv{};
        iv.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        iv.image = swapImages[i];
        iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv.format = swapFormat;
        iv.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                          VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
        iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        iv.subresourceRange.levelCount = 1;
        iv.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &iv, nullptr, &swapImageViews[i]);
    }

    framebuffers.resize(swapImageViews.size());
    for(size_t i = 0; i < swapImageViews.size(); i++) {
        VkFramebufferCreateInfo fb{};
        fb.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb.renderPass = renderPass;
        fb.attachmentCount = 2;
        VkImageView attachments[] = {
            swapImageViews[i],
            depthImageView
        };
        fb.width = swapExtent.width;
        fb.height = swapExtent.height;
        fb.layers = 1;
        vkCreateFramebuffer(device, &fb, nullptr, &framebuffers[i]);
    }

    vkResetCommandBuffer(cmd, 0);
}


VkDescriptorSetLayout textureLayout;

void CreateTextureDescriptorLayout() {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings = &binding;

    vkCreateDescriptorSetLayout(device, &info, nullptr, &textureLayout);
}

VkDescriptorPool descriptorPool;

void CreateTextureDescriptorPool(uint32_t maxTextures) {
    VkDescriptorPoolSize size{};
    size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    size.descriptorCount = maxTextures;

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = 1;
    info.pPoolSizes = &size;
    info.maxSets = maxTextures;

    vkCreateDescriptorPool(device, &info, nullptr, &descriptorPool);
}

#include "Vulkan/VKPipeline.hpp"

void InitVulkan(HWND hwnd)
{
    CreateInstance();
    CreateSurface(hwnd);
    CreateDevice();

    VmaAllocatorCreateInfo info{};
    info.instance       = instance;
    info.physicalDevice = physicalDevice;
    info.device         = device;
    info.vulkanApiVersion = VK_API_VERSION_1_1;

    VkResult res = vmaCreateAllocator(&info, &vma);

    CreateSwapchain(CCEGLView::get()->getFrameSize().width, CCEGLView::get()->getFrameSize().height);

    VkFenceCreateInfo fci{};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(device, &fci, nullptr, &inFlightFence);

    VkCommandPoolCreateInfo cpci{};
    cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.queueFamilyIndex = queueIndex;
    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(device, &cpci, nullptr, &cmdPool);

    VkCommandBufferAllocateInfo cbai{};
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool = cmdPool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    vkAllocateCommandBuffers(device, &cbai, &cmd);


    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(device, &semInfo, nullptr, &imageAvailable);
    vkCreateSemaphore(device, &semInfo, nullptr, &renderFinished);

    swapImageViews.resize(swapImages.size());
    for (size_t i = 0; i < swapImages.size(); i++) {
        VkImageViewCreateInfo iv{};
        iv.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        iv.image = swapImages[i];
        iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv.format = swapFormat;
        iv.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        iv.subresourceRange.levelCount = 1;
        iv.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &iv, nullptr, &swapImageViews[i]);
    }

    VkAttachmentDescription color{};
    color.format = swapFormat;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth{};
    depth.format = VK_FORMAT_D32_SFLOAT;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 1;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    VkRenderPassCreateInfo rpci{};
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = 2;
    VkAttachmentDescription attachments[] = { color, depth };
    rpci.pAttachments = attachments;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;

    vkCreateRenderPass(device, &rpci, nullptr, &renderPass);

    framebuffers.resize(swapImageViews.size());
    for (size_t i = 0; i < swapImageViews.size(); i++) {
        VkFramebufferCreateInfo fb{};
        fb.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb.renderPass = renderPass;
        fb.attachmentCount = 2;
        VkImageView attachments[] = {
            swapImageViews[i],
            depthImageView
        };
        fb.pAttachments = attachments;
        fb.width = swapExtent.width;
        fb.height = swapExtent.height;
        fb.layers = 1;
        vkCreateFramebuffer(device, &fb, nullptr, &framebuffers[i]);
    }

    CreateTextureDescriptorLayout();
    CreateTextureDescriptorPool(4096 * 4);

    log::error("Vulkan initialised!");
}

HWND getWindowHandle()
{
    return WindowFromDC(wglGetCurrentDC());
}


bool VulkanRenderer::init()
{
    static bool binded = false;

    if (!binded)
    {
        vkbInit(nullptr);
        binded = true;
    }

    HWND hwnd = getWindowHandle();
    InitVulkan(hwnd);

    return true;
}

uint32_t imageIndex;
#include "Vulkan/VKShader.hpp"

bool VulkanRenderer::begin()
{
    drawOrder = 0;
    drawCalls = 0;
    bindCalls = 0;
    bindTextureCalls = 0;

    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &imageIndex);
    // vkQueueWaitIdle(graphicsQueue);
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFence);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        log::info("error: {}", (int)result);
        log::info("size: {}", CCEGLView::get()->getFrameSize());
        recreateSwapchain(CCEGLView::get()->getFrameSize().width, CCEGLView::get()->getFrameSize().height);
        return false;
    }

    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &bi);

    VkClearValue clear[2]{};
    clear[0].color = { {0, 0, 0, 0.0f} };
    clear[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo rp{};
    rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp.renderPass = renderPass;
    rp.framebuffer = framebuffers[imageIndex];
    rp.renderArea.offset = {0,0};
    rp.renderArea.extent = swapExtent;
    rp.clearValueCount = 2;
    rp.pClearValues = clear;

    vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);

    return true;
}

void VulkanRenderer::present()
{
    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &imageAvailable;
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &renderFinished;

    vkQueueSubmit(graphicsQueue, 1, &submit, inFlightFence);

    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &renderFinished;
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain;
    present.pImageIndices = &imageIndex;

    vkQueuePresentKHR(graphicsQueue, &present);
}

#include <Geode/cocos/kazmath/include/kazmath/kazmath.h>

#include <Geode/modify/CCSprite.hpp>
#include <Geode/modify/CCLayerColor.hpp>
#include <Geode/modify/CCSpriteBatchNode.hpp>
#include <Geode/modify/CCTexture2D.hpp>
#include <Geode/modify/CCDrawNode.hpp>
#include <Geode/modify/CCGLProgram.hpp>
#include <Geode/modify/CCEGLView.hpp>
#include "VKCocosOwnedMemory.hpp"

class $modify (CCGLProgram)
{
    bool initWithVertexShaderByteArray(const GLchar* vShaderByteArray, const GLchar* fShaderByteArray)
    {
        log::info("vertex: \n\n{}\n\nfrag: \n\n{}", vShaderByteArray, fShaderByteArray);

        return CCGLProgram::initWithVertexShaderByteArray(vShaderByteArray, fShaderByteArray);
    }
};

class $modify (CCEGLView)
{
    void resizeWindow(int width, int height)
    {
        CCEGLView::resizeWindow(width, height);

        recreateSwapchain(width, height);
    }
};

class $modify (VKDrawNode, CCDrawNode)
{
    struct Fields
    {
        VKCocosOwnedMemory* mem = nullptr;
    };

    virtual bool init()
    {
        if (!CCDrawNode::init())
            return false;

        m_fields->mem = VKCocosOwnedMemory::create(m_uBufferCapacity * sizeof(ccV2F_C4B_T2F));
        m_fields->mem->retain();

        return true;
    }

    void destructor()
    {
        m_fields->mem->release();
        CCDrawNode::~CCDrawNode();
    }

    void render()
    {
        auto blendPipeline = VKPipeline::get<ccV2F_C4B_T2F>(getBlendFunc());        
        auto mvp = getNodeToWorldTransform(this);

        if (m_bDirty)
        {
            m_fields->mem->resize(m_uBufferCapacity*sizeof(ccV2F_C4B_T2F));
            m_fields->mem->upload(m_pBuffer, m_nBufferCount * sizeof(ccV2F_C4B_T2F));
            m_bDirty = false;
        }

        vkCmdPushConstants(cmd,
            blendPipeline->getLayout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(kmMat4),
            &mvp);

        blendPipeline->bind();

        VkBuffer buffers[] = { m_fields->mem->getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

        updateScissor();
        INCREMENT_DRAW_CALLS(1);
        vkCmdDraw(cmd, m_nBufferCount, 1, 0, 0);
    }
};

void updateScissor()
{
    if (CCEGLView::get()->isScissorEnabled())
    {
        auto rect = CCEGLView::get()->getScissorRect();
        auto scale = 1.0f / CCDirector::get()->getWinSize().width * swapExtent.width;

        VkRect2D scissor = {};
        scissor.extent = {
            (uint32_t)(int)(rect.size.width * scale),
            (uint32_t)(int)(rect.size.height * scale)
        };
        scissor.offset = {
            (int)(rect.origin.x * scale),
            (int)(swapExtent.height - scissor.extent.height - rect.origin.y * scale)
        };
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }
    else
    {
        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = swapExtent;
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }
}

float getFakeZ()
{
    drawOrder++;
    return -1.0f + (drawOrder * 0.000001f);
}

kmMat4 getNodeToWorldTransform(cocos2d::CCNode* node)
{
    #ifdef DEPTH_BUFFER_EXPERIMENT
    kmMat4 model;
    kmGLGetMatrix(KM_GL_MODELVIEW, &model);

    kmMat4 projection;
    kmGLGetMatrix(KM_GL_PROJECTION, &projection);

    kmMat4 identity;
    kmMat4Identity(&identity);
    identity.mat[14] = getFakeZ();
    
    kmMat4 temp;
    kmMat4Multiply(&temp, &identity, &model);

    kmMat4 mvp;
    kmMat4Multiply(&mvp, &projection, &temp);
    #else

    kmMat4 model;
    kmGLGetMatrix(KM_GL_MODELVIEW, &model);

    kmMat4 projection;
    kmGLGetMatrix(KM_GL_PROJECTION, &projection);

    kmMat4 mvp;
    kmMat4Multiply(&mvp, &projection, &model);
    #endif

    return mvp;
}

std::vector<DrawCommand> drawCommands = {};

void VulkanRenderer::end()
{
    if (drawCommands.empty())
        return;
    
    std::sort(drawCommands.begin(), drawCommands.end(),
    [](const DrawCommand& a, const DrawCommand& b)
    {
        if (a.texture != b.texture)
            return a.texture < b.texture;

        if (a.func.src != b.func.src)
            return a.func.src < b.func.src;

        if (a.func.dst != b.func.dst)
            return a.func.dst < b.func.dst;

        if (a.scissorEnabled != b.scissorEnabled)
            return a.scissorEnabled < b.scissorEnabled;

        return false;
    });

    auto current = drawCommands[0];

    auto pipeline = VKPipeline::get<ccV3F_C4B_T2F_Quad>(
        current.func,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
    );

    pipeline->bind();
    pipeline->bindTexture(current.texture);

    updateScissor();

    for (size_t i = 0; i < drawCommands.size(); i++)
    {
        auto& cmdData = drawCommands[i];

        bool stateChanged =
            cmdData.texture != current.texture ||
            cmdData.func.src != current.func.src ||
            cmdData.func.dst != current.func.dst ||
            cmdData.scissorEnabled != current.scissorEnabled;

        if (stateChanged)
        {
            current = cmdData;

            pipeline = VKPipeline::get<ccV3F_C4B_T2F_Quad>(
                current.func,
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
            );

            pipeline->bind();
            pipeline->bindTexture(current.texture);
        }

        VkBuffer buffers[] = { cmdData.memory->getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

        vkCmdPushConstants(cmd,
            pipeline->getLayout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(kmMat4),
            &cmdData.mat);

        if (cmdData.scissorEnabled)
        {
            auto rect = cmdData.scissorRect;
            float scale = 1.0f / CCDirector::get()->getWinSize().width * swapExtent.width;

            VkRect2D scissor{};
            scissor.extent = {
                (uint32_t)(rect.size.width * scale),
                (uint32_t)(rect.size.height * scale)
            };
            scissor.offset = {
                (int)(rect.origin.x * scale),
                (int)(swapExtent.height - scissor.extent.height - rect.origin.y * scale)
            };

            vkCmdSetScissor(cmd, 0, 1, &scissor);
        }

        INCREMENT_DRAW_CALLS(1);
        vkCmdDraw(cmd, 4, 1, 0, 0);
    }

    drawCommands.clear();
}

void handleDrawCommand(cocos2d::CCSprite* sprite, VKCocosOwnedMemory* memory)
{
    drawCommands.push_back({
        getNodeToWorldTransform(sprite),
        sprite->getBlendFunc(),
        sprite->getTexture(),
        memory,
        CCEGLView::get()->isScissorEnabled(),
        CCEGLView::get()->getScissorRect()
    });
}