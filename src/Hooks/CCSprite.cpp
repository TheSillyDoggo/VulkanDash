#include <Geode/Geode.hpp>
#include <Geode/modify/CCSprite.hpp>
#include "../vkbind.h"
#include "../VKCocosOwnedMemory.hpp"
#include "../VulkanRenderer.hpp"
#include "../Vulkan/VKPipeline.hpp"
#include "VKTexture2D.hpp"

using namespace geode::prelude;

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
        
        m_fields->memory->upload(&m_sQuad, sizeof(ccV3F_C4B_T2F_Quad));
    }

    virtual bool initWithTexture(CCTexture2D *pTexture, const CCRect& rect, bool rotated)
    {
        if (!CCSprite::initWithTexture(pTexture, rect, rotated))
            return false;
        
        m_fields->memory = VKCocosOwnedMemory::create(sizeof(ccV3F_C4B_T2F_Quad));
        m_fields->memory->retain();
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
        #ifdef DEPTH_BUFFER_EXPERIMENT
        handleDrawCommand(this, m_fields->memory);
        return;
        #endif

        auto blendPipeline = VKPipeline::get<ccV3F_C4B_T2F_Quad>(getBlendFunc(), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
        auto mvp = getNodeToWorldTransform(this);

        blendPipeline->bind();

        VkBuffer buffers[] = { m_fields->memory->getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

        vkCmdPushConstants(cmd,
            blendPipeline->getLayout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(kmMat4),
            &mvp);

        blendPipeline->bindTexture(getTexture());

        updateScissor();
        INCREMENT_DRAW_CALLS(1);
        vkCmdDraw(cmd, 4, 1, 0, 0);
    }
};