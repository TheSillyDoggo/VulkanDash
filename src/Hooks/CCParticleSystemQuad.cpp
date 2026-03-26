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
        auto pipeline = VKPipeline::get<ccV3F_C4B_T2F>(getBlendFunc());
        
        auto mvp = getNodeToWorldTransform(this);
        ccV3F_C4B_T2F out[m_uTotalParticles * 6 * sizeof(ccV3F_C4B_T2F)];

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

                out[b].texCoords = point.texCoords;
                out[b].colors = point.colors;
                out[b].vertices.x = point.vertices.x;
                out[b].vertices.y = point.vertices.y;
            }
        }

        m_fields->memory->resize(m_uTotalParticles * 6 * sizeof(ccV3F_C4B_T2F));
        m_fields->memory->upload(out, m_uTotalParticles * 6 * sizeof(ccV3F_C4B_T2F));

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

        updateScissor();
        vkCmdDraw(cmd, m_uTotalParticles * 6, 1, 0, 0);
    }
};