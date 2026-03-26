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
}

void recreateSwapchain(uint32_t width, uint32_t height) {
    vkDeviceWaitIdle(device);

    for(auto fb : framebuffers)
        vkDestroyFramebuffer(device, fb, nullptr);

    for(auto iv : swapImageViews)
        vkDestroyImageView(device, iv, nullptr);

    vkDestroySwapchainKHR(device, swapchain, nullptr);

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
        fb.attachmentCount = 1;
        fb.pAttachments = &swapImageViews[i];
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

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkRenderPassCreateInfo rpci{};
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = 1;
    rpci.pAttachments = &color;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;

    vkCreateRenderPass(device, &rpci, nullptr, &renderPass);

    framebuffers.resize(swapImageViews.size());
    for (size_t i = 0; i < swapImageViews.size(); i++) {
        VkFramebufferCreateInfo fb{};
        fb.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb.renderPass = renderPass;
        fb.attachmentCount = 1;
        fb.pAttachments = &swapImageViews[i];
        fb.width = swapExtent.width;
        fb.height = swapExtent.height;
        fb.layers = 1;
        vkCreateFramebuffer(device, &fb, nullptr, &framebuffers[i]);
    }

    CreateTextureDescriptorLayout();
    CreateTextureDescriptorPool(4096 * 4);

    VmaAllocatorCreateInfo info{};
    info.instance       = instance;
    info.physicalDevice = physicalDevice;
    info.device         = device;
    info.vulkanApiVersion = VK_API_VERSION_1_1;

    VkResult res = vmaCreateAllocator(&info, &vma);

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
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &imageIndex);
    vkQueueWaitIdle(graphicsQueue);

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

    VkClearValue clear{};
    clear.color = { {0, 0, 0, 0.0f} };

    VkRenderPassBeginInfo rp{};
    rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp.renderPass = renderPass;
    rp.framebuffer = framebuffers[imageIndex];
    rp.renderArea.offset = {0,0};
    rp.renderArea.extent = swapExtent;
    rp.clearValueCount = 1;
    rp.pClearValues = &clear;

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

kmMat4 getNodeToWorldTransform(cocos2d::CCNode* node)
{
    kmMat4 model;
    kmGLGetMatrix(KM_GL_MODELVIEW, &model);

    kmMat4 projection;
    kmGLGetMatrix(KM_GL_PROJECTION, &projection);

    kmMat4 mvp;
    kmMat4Multiply(&mvp, &projection, &model);

    return mvp;
}

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
        m_fields->mem = VKCocosOwnedMemory::create(512 * sizeof(VkSpriteVertex));
        m_fields->mem->retain();
        return CCDrawNode::init();
    }

    void destructor()
    {
        m_fields->mem->release();
        CCDrawNode::~CCDrawNode();
    }

    void render()
    {
        auto blendPipeline = VKPipeline::get<VkSpriteVertex>(getBlendFunc());
        int v = m_nBufferCount;
        
        auto mvp = getNodeToWorldTransform(this);
        VkSpriteVertex out[m_uBufferCapacity];

        m_fields->mem->resize(m_uBufferCapacity*sizeof(VkSpriteVertex));

        for (size_t i = 0; i < v; i++)
        {
            ccV2F_C4B_T2F point = *(ccV2F_C4B_T2F *)(m_pBuffer + (i));

            int b = i;

            out[b].pos[0] = point.vertices.x;
            out[b].pos[1] = point.vertices.y;
            out[b].pos[2] = 0;

            out[b].color[0] = point.colors.r / 255.0f;
            out[b].color[1] = point.colors.g / 255.0f;
            out[b].color[2] = point.colors.b / 255.0f;
            out[b].color[3] = point.colors.a / 255.0f;

            out[b].uv[0] = 0;
            out[b].uv[1] = 0;
        }

        m_fields->mem->upload(out, m_uBufferCapacity * sizeof(VkSpriteVertex));

        vkCmdPushConstants(cmd,
            blendPipeline->getLayout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(kmMat4),
            &mvp);

        vkCmdBindPipeline(cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            blendPipeline->getPipeline());

        VkBuffer buffers[] = { m_fields->mem->getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

        VkDescriptorSet desc = static_cast<VKTexture2D*>(CCTextureCache::get()->addImage("cc_2x2_white_image", true))->getDescriptor();
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            blendPipeline->getLayout(),
            0,              // set = 0
            1,
            &desc,
            0,
            nullptr
        );

        updateScissor();
        vkCmdDraw(cmd, m_nBufferCount, 1, 0, 0);
    }

    void ensureCapacity(unsigned int count)
    {
        log::info("CCDrawNode::enusreCapacity: {}", count);

        if(m_nBufferCount + count > m_uBufferCapacity)
        {
            m_uBufferCapacity += std::max<unsigned int>(m_uBufferCapacity, count);
            m_pBuffer = (ccV2F_C4B_T2F*)realloc(m_pBuffer, m_uBufferCapacity*sizeof(ccV2F_C4B_T2F));
            m_fields->mem->resize(m_uBufferCapacity*sizeof(ccV2F_C4B_T2F));
        }

        // CCDrawNode::ensureCapacity(count);
    }
};

class $modify (VKSprite, CCSprite)
{
    struct Fields
    {
        VKCocosOwnedMemory* memory = nullptr;
    };

    void updateMemory()
    {
        if (!m_fields->memory)
            return;

        if (m_pobBatchNode)
            return;

        ccV3F_C4B_T2F out[6];

        out[0] = m_sQuad.bl;
        out[1] = m_sQuad.tl;
        out[2] = m_sQuad.br;
        out[3] = m_sQuad.br;
        out[4] = m_sQuad.tr;
        out[5] = m_sQuad.tl;
        
        m_fields->memory->upload(out, sizeof(ccV3F_C4B_T2F) * 6);
    }

    virtual bool initWithTexture(CCTexture2D *pTexture, const CCRect& rect, bool rotated)
    {
        if (!CCSprite::initWithTexture(pTexture, rect, rotated))
            return false;
        
        m_fields->memory = VKCocosOwnedMemory::create(sizeof(ccV3F_C4B_T2F) * 6);
        m_fields->memory->retain();
        // this->addTether(m_fields->memory);
        updateMemory();

        return true;
    }

    void destructor()
    {
        m_fields->memory->release();
        CCSprite::~CCSprite();
    }

    void updateColor(void)
    {
        CCSprite::updateColor();
        updateMemory();
    }

    virtual void setTextureRect(const CCRect& rect, bool rotated, const CCSize& untrimmedSize)
    {
        CCSprite::setTextureRect(rect, rotated, untrimmedSize);
        updateMemory();
    }

    virtual void draw()
    {
        auto blendPipeline = VKPipeline::get<ccV3F_C4B_T2F>(getBlendFunc());
        auto mvp = getNodeToWorldTransform(this);

        vkCmdBindPipeline(cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            blendPipeline->getPipeline());

        VkBuffer buffers[] = { m_fields->memory->getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

        vkCmdPushConstants(cmd,
            blendPipeline->getLayout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(kmMat4),
            &mvp);
        
        VkDescriptorSet desc = static_cast<VKTexture2D*>(getTexture())->getDescriptor();

        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            blendPipeline->getLayout(),
            0,              // set = 0
            1,
            &desc,
            0,
            nullptr
        );

        updateScissor();
        vkCmdDraw(cmd, 6, 1, 0, 0);
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