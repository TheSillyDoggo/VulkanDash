#include <Geode/Geode.hpp>
#include <Geode/modify/CCParticleSystemQuad.hpp>
#include "../vkbind.h"
#include "../VKCocosOwnedMemory.hpp"
#include "../VulkanRenderer.hpp"
#include "../Vulkan/VKPipeline.hpp"
#include "VKTexture2D.hpp"

using namespace geode::prelude;

cocos2d::ccBlendFunc getCurrentBlendFunc()
{
    GLint blendSrcRGB, blendDstRGB, blendSrcAlpha, blendDstAlpha;

    glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstAlpha);

    return ccBlendFunc(blendSrcRGB, blendDstRGB);
}

cocos2d::ccColor4B getCurrentColour()
{
    GLfloat color[4];
    glGetFloatv(GL_CURRENT_COLOR, color);

    float r = color[0];
    float g = color[1];
    float b = color[2];
    float a = color[3];

    return ccc4(r * 255, g * 255, b * 255, a * 255);
}

float getCurrentLineWidth()
{
    GLfloat lineWidth;
    glGetFloatv(GL_LINE_WIDTH, &lineWidth);

    return lineWidth;
}

// really bad
/*void myDrawCircle(const cocos2d::CCPoint& center, float radius, float angle, unsigned int segments, bool drawLineToCenter, float scaleX, float scaleY)
{
    static VKCocosOwnedMemory* memory = []{
        auto mem = VKCocosOwnedMemory::create(512);
        CCPoolManager::sharedPoolManager()->removeObject(mem);
        return mem;
    }();

    const float coef = 2.0f * (float)M_PI / segments;
    auto col = getCurrentColour();
    float width = getCurrentLineWidth();
    float halfWidth = width * 0.5f;

    std::vector<CCPoint> points(segments + 1);

    for (unsigned int i = 0; i <= segments; i++)
    {
        float rads = i * coef;
        float x = radius * cosf(rads + angle) * scaleX + center.x;
        float y = radius * sinf(rads + angle) * scaleY + center.y;
        points[i] = ccp(x, y);
    }

    std::vector<ccV3F_C4B_T2F> out;
    out.reserve(segments * 6);

    for (unsigned int i = 0; i < segments; i++)
    {
        CCPoint p0 = points[i];
        CCPoint p1 = points[i + 1];

        CCPoint dir = ccpSub(p1, p0);
        float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
        if (len == 0) continue;

        dir.x /= len;
        dir.y /= len;

        CCPoint normal = ccp(-dir.y, dir.x);
        CCPoint offset = ccpMult(normal, halfWidth);

        CCPoint v0 = ccpSub(p0, offset);
        CCPoint v1 = ccpAdd(p0, offset);
        CCPoint v2 = ccpSub(p1, offset);
        CCPoint v3 = ccpAdd(p1, offset);

        out.push_back({ { v0.x, v0.y, 0 }, col, {0,0} });
        out.push_back({ { v1.x, v1.y, 0 }, col, {0,0} });
        out.push_back({ { v2.x, v2.y, 0 }, col, {0,0} });
        out.push_back({ { v2.x, v2.y, 0 }, col, {0,0} });
        out.push_back({ { v1.x, v1.y, 0 }, col, {0,0} });
        out.push_back({ { v3.x, v3.y, 0 }, col, {0,0} });
    }

    memory->resize(out.size() * sizeof(ccV3F_C4B_T2F));
    memory->upload(out.data(), out.size() * sizeof(ccV3F_C4B_T2F));

    auto pipeline = VKPipeline::get<ccV3F_C4B_T2F>(
        getCurrentBlendFunc(),
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    );

    auto mvp = getNodeToWorldTransform(nullptr);

    vkCmdPushConstants(cmd,
        pipeline->getLayout(),
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(kmMat4),
        &mvp);

    pipeline->bind();
    pipeline->bindTexture(CCTextureCache::get()->addImage("cc_2x2_white_image", true));

    VkBuffer buffers[] = { memory->getBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

    updateScissor();
    INCREMENT_DRAW_CALLS(1);

    vkCmdDraw(cmd, static_cast<uint32_t>(out.size()), 1, 0, 0);
}

$execute {
    (void)Mod::get()->hook(
        reinterpret_cast<void*>(
            // All of this is to get the address of ccDrawCircle
            geode::addresser::getNonVirtual(
                // This is used because this function is overloaded,
                // otherwise just a regular function pointer would suffice (&foobar)
                geode::modifier::Resolve<const cocos2d::CCPoint&, float, float, unsigned int, bool, float, float>::func(&cocos2d::ccDrawCircle)
            )
        ),
        &myDrawCircle, // Our detour
        "cocos2d::ccDrawCircle", // Display name, shows up on the console
        tulip::hook::TulipConvention::Cdecl // Static free-standing cocos2d functions are cdecl
    );
}*/