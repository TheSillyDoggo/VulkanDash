#include <Geode/Geode.hpp>
#include <Geode/modify/CCTextureAtlas.hpp>
#include <Geode/modify/CCSpriteBatchNode.hpp>
#include <Geode/modify/CCSprite.hpp>
#include "../vkbind.h"
#include "../VKCocosOwnedMemory.hpp"
#include "../VulkanRenderer.hpp"
#include "../Vulkan/VKPipeline.hpp"
#include "VKTexture2D.hpp"

using namespace geode::prelude;

std::unordered_map<unsigned int, VKCocosOwnedMemory*> memories = {};
CCSpriteBatchNode* probablyTheLastBatchNode = nullptr;

class $modify (CCSpriteBatchNode)
{
    virtual void draw()
    {
        probablyTheLastBatchNode = this;

        CCSpriteBatchNode::draw();
    }
};

class $modify (VKTextureAtlas, CCTextureAtlas)
{
    VKCocosOwnedMemory* getMemory()
    {       
        if (!memories.contains(m_uID))
        {
            memories[m_uID] = VKCocosOwnedMemory::create(8);
            memories[m_uID]->retain();
        }

        return memories[m_uID];
    }

    bool resizeCapacity(unsigned int n)
    {
        auto ret = CCTextureAtlas::resizeCapacity(n);
        // getMemory()->resize();
        return ret;
    }

    void drawQuads()
    {
        auto pipeline = VKPipeline::get<ccV3F_C4B_T2F>(probablyTheLastBatchNode ? probablyTheLastBatchNode->getBlendFunc() : ccBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA}));
        auto mvp = getNodeToWorldTransform(nullptr);
        // getMemory()->resize(sizeof(ccV3F_C4B_T2F) * 4 * getTotalQuads());
        // getMemory()->upload(m_pQuads, sizeof(ccV3F_C4B_T2F) * 4 * getTotalQuads());

        auto mem = getMemory();
        mem->resize(sizeof(ccV3F_C4B_T2F) * 6 * getCapacity());
        auto out = reinterpret_cast<ccV3F_C4B_T2F*>(mem->getMapped());

        for (size_t i = 0; i < getCapacity(); i++)
        {
            auto quad = m_pQuads[i];

            size_t b = i * 6;

            out[b + 0] = quad.bl;
            out[b + 1] = quad.br;
            out[b + 2] = quad.tl;
            out[b + 3] = quad.tr;
            out[b + 4] = quad.tl;
            out[b + 5] = quad.br;
        }
        
        vkCmdPushConstants(cmd,
            pipeline->getLayout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(kmMat4),
            &mvp);

        pipeline->bind();
        pipeline->bindTexture(getTexture());

        VkBuffer buffers[] = { getMemory()->getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

        updateScissor();
        INCREMENT_DRAW_CALLS(1);
        vkCmdDraw(cmd, 6 * getTotalQuads(), 1, 0, 0);
    }
};

class $modify (CCSprite)
{
    /*virtual void updateTransform(void)
    {
        bool dirty = isDirty();

        CCSprite::updateTransform();

        if (m_pobBatchNode && dirty)
        {
            auto mem = static_cast<VKTextureAtlas*>(m_pobBatchNode->getTextureAtlas())->getMemory();
            mem->resize(sizeof(ccV3F_C4B_T2F) * 6 * m_pobBatchNode->getTextureAtlas()->getCapacity());
            auto out = reinterpret_cast<ccV3F_C4B_T2F*>(mem->getMapped());

            size_t base = m_uAtlasIndex * 6;

            if (m_uAtlasIndex < 0 || m_uAtlasIndex > m_pobTextureAtlas->getCapacity())
            {
                return;
            }

            out[base + 0] = m_sQuad.bl;
            out[base + 1] = m_sQuad.tl;
            out[base + 2] = m_sQuad.br;
            out[base + 3] = m_sQuad.br;
            out[base + 4] = m_sQuad.tr;
            out[base + 5] = m_sQuad.tl;
        }
    }*/
};