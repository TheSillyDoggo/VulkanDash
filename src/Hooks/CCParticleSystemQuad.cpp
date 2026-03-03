#include <Geode/Geode.hpp>
#include <Geode/modify/CCParticleSystemQuad.hpp>
#include "../vkbind.h"
#include "../VKCocosOwnedMemory.hpp"
#include "../VulkanRenderer.hpp"
#include "../Vulkan/VKPipeline.hpp"
#include "VKTexture2D.hpp"

using namespace geode::prelude;

class $modify (VKParticleSystemQuad, CCParticleSystemQuad)
{
    struct Fields
    {
        VKCocosOwnedMemory* memory = nullptr;
    };

    virtual bool initWithTotalParticles(unsigned int numberOfParticles, bool unk)
    {
        m_fields->memory = VKCocosOwnedMemory::create(sizeof(ccV3F_C4B_T2F) * 6 * 16, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        m_fields->memory->retain();

        return CCParticleSystemQuad::initWithTotalParticles(numberOfParticles, unk);
    }
    
    void destructor()
    {
        m_fields->memory->release();

        CCParticleSystemQuad::~CCParticleSystemQuad();
    }

    virtual void draw()
    {
        auto pipeline = VKPipeline::get<VkSpriteVertex>(getBlendFunc());
        
        auto mvp = getNodeToWorldTransform(this);
        VkSpriteVertex out[m_uTotalParticles * 6 * sizeof(ccV2F_C4B_T2F)];

        const CCPoint v[6] = {
            ccp(0, 0), ccp(0, 1), ccp(1, 0), ccp(1, 0), ccp(1, 1), ccp(0, 1),
        };

        for (size_t i = 0; i < m_uTotalParticles; i++)
        {
            ccV3F_C4B_T2F_Quad quad = *(ccV3F_C4B_T2F_Quad *)(m_pQuads + (i));

            for (size_t c = 0; c < 6; c++)
            {
                ccV3F_C4B_T2F point = quad.bl;

                if (v[c] == ccp(1, 0))
                    point = quad.br;

                if (v[c] == ccp(0, 1))
                    point = quad.tl;

                if (v[c] == ccp(1, 1))
                    point = quad.tr;

                int b = i * 6 + c;

                out[b].pos[0] = point.vertices.x;
                out[b].pos[1] = point.vertices.y;
                out[b].pos[2] = 0;

                out[b].color[0] = point.colors.r / 255.0f;
                out[b].color[1] = point.colors.g / 255.0f;
                out[b].color[2] = point.colors.b / 255.0f;
                out[b].color[3] = point.colors.a / 255.0f;

                out[b].uv[0] = v[c].x;
                out[b].uv[1] = v[c].y;

                // out[b].uv[0] = m_tTextureRect.origin.x / (float)getTexture()->getPixelsWide() + v[c].x * (m_tTextureRect.size.width / (float)getTexture()->getPixelsWide());
                // out[b].uv[1] = m_tTextureRect.origin.y / (float)getTexture()->getPixelsHigh() + v[c].y * (m_tTextureRect.size.height / (float)getTexture()->getPixelsHigh());
            }
        }

        m_fields->memory->resize(m_uTotalParticles * 6 * sizeof(VkSpriteVertex));
        m_fields->memory->upload(out, m_uTotalParticles * 6 * sizeof(VkSpriteVertex));

        vkCmdPushConstants(cmd,
            pipeline->getLayout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(kmMat4),
            &mvp);

        vkCmdBindPipeline(cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline->getPipeline());

        VkBuffer buffers[] = { m_fields->memory->getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

        VkDescriptorSet desc = static_cast<VKTexture2D*>(getTexture())->getDescriptor();
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline->getLayout(),
            0,              // set = 0
            1,
            &desc,
            0,
            nullptr
        );

        vkCmdDraw(cmd, m_uTotalParticles * 6, 1, 0, 0);
    }
};