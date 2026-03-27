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
        m_fields->memory = VKCocosOwnedMemory::create(sizeof(ccV3F_C4B_T2F) * 6 * numberOfParticles, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
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

        m_fields->memory->resize(m_uParticleIdx * 6 * sizeof(ccV3F_C4B_T2F));
        m_fields->memory->upload(m_pQuads, m_uParticleIdx * 6 * sizeof(ccV3F_C4B_T2F));

        vkCmdPushConstants(cmd,
            pipeline->getLayout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(kmMat4),
            &mvp);

        pipeline->bind();
        pipeline->bindTexture(getTexture());

        VkBuffer buffers[] = { m_fields->memory->getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

        updateScissor();
        INCREMENT_DRAW_CALLS(1);
        vkCmdDraw(cmd, m_uParticleIdx * 6, 1, 0, 0);
    }
};