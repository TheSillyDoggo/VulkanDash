#include <Geode/Geode.hpp>
#include <Geode/modify/CCLayerColor.hpp>
#include "../vkbind.h"
#include "../VKCocosOwnedMemory.hpp"
#include "../VulkanRenderer.hpp"
#include "../Vulkan/VKPipeline.hpp"
#include "VKTexture2D.hpp"

using namespace geode::prelude;

class $modify (VKLayerColor, CCLayerColor)
{
    struct Fields
    {
        VKCocosOwnedMemory* memory = nullptr;
    };

    virtual bool initWithColor(const ccColor4B& color, GLfloat width, GLfloat height)
    {
        m_fields->memory = VKCocosOwnedMemory::create(sizeof(VkSpriteVertex) * 6);
        m_fields->memory->retain();

        return CCLayerColor::initWithColor(color, width, height);
    }
    
    void destructor()
    {
        m_fields->memory->release();

        CCLayerColor::~CCLayerColor();
    }

    virtual void updateColor()
    {
        CCLayerColor::updateColor();
        updateMemory();
    }

    virtual void setContentSize(const CCSize & var)
    {
        CCLayerColor::setContentSize(var);
        updateMemory();
    }

    void updateMemory()
    {
        VkSpriteVertex out[6];

        const CCPoint v[6] = {
            ccp(0, 0), ccp(0, 1), ccp(1, 0), ccp(0, 1), ccp(1, 1), ccp(1, 0),
        };
        
        for (int i = 0; i < 6; i++)
        {
            out[i].pos[0] = v[i].x * m_pSquareVertices[3].x;
            out[i].pos[1] = v[i].y * m_pSquareVertices[3].y;
            out[i].pos[2] = 0;

            if (v[i] == ccp(0, 0))
            {
                out[i].color[0] = m_pSquareColors[0].r;
                out[i].color[1] = m_pSquareColors[0].g;
                out[i].color[2] = m_pSquareColors[0].b;
                out[i].color[3] = m_pSquareColors[0].a;
            }

            if (v[i] == ccp(1, 0))
            {
                out[i].color[0] = m_pSquareColors[1].r;
                out[i].color[1] = m_pSquareColors[1].g;
                out[i].color[2] = m_pSquareColors[1].b;
                out[i].color[3] = m_pSquareColors[1].a;
            }

            if (v[i] == ccp(0, 1))
            {
                out[i].color[0] = m_pSquareColors[2].r;
                out[i].color[1] = m_pSquareColors[2].g;
                out[i].color[2] = m_pSquareColors[2].b;
                out[i].color[3] = m_pSquareColors[2].a;
            }

            if (v[i] == ccp(1, 1))
            {
                out[i].color[0] = m_pSquareColors[3].r;
                out[i].color[1] = m_pSquareColors[3].g;
                out[i].color[2] = m_pSquareColors[3].b;
                out[i].color[3] = m_pSquareColors[3].a;
            }

            out[i].uv[0] = 0;
            out[i].uv[1] = 0;
        }

        m_fields->memory->upload(out, sizeof(out));
    }

    virtual void draw()
    {
        auto pipeline = VKPipeline::get<VkSpriteVertex>(getBlendFunc());
        pipeline->bind();

        auto mvp = getNodeToWorldTransform(this);

        VkBuffer buffers[] = { m_fields->memory->getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

        vkCmdPushConstants(cmd,
            pipeline->getLayout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(kmMat4),
            &mvp);

        pipeline->bindTexture(CCTextureCache::get()->addImage("cc_2x2_white_image", true));

        updateScissor();
        INCREMENT_DRAW_CALLS(1);
        vkCmdDraw(cmd, 6, 1, 0, 0);
    }
};