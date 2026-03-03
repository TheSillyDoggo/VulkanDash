#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/CCSprite.hpp>
#include "OpenGLRenderer.hpp"

using namespace geode::prelude;

/*GenericRenderer* getRenderer()
{
    static GenericRenderer* instance = nullptr;

    if (!instance)
        instance = new OpenGLRenderer();

    return instance;
}

$on_mod(Loaded)
{
    // CCDirector::get()->m_bDisplayStats = true;
};

class $modify (CCDirector)
{
    void showFPSLabel()
    {
        kmGLPushMatrix();
        getRenderer()->draw();
        kmGLPopMatrix();

        m_pFPSNode->setString(fmt::format("FPS: {}", (int)(1.0f / getDeltaTime())).c_str());
        m_pFPSNode->visit();
        // CCDirector::showFPSLabel(8);
    }

    /*void showStats()
    {
        /*if (!m_pFPSNode)
            return;

        unsigned int calls = *reinterpret_cast<unsigned int*>(geode::base::getCocos() + 0x1a9b20);
        m_pFPSNode->setString(fmt::format("\nGL draw calls: {}", calls).c_str());
        // m_pFPSNode->visit();* /

        CCDirector::showStats();

        getRenderer()->draw();
    }* /
};

class $modify (CCSprite)
{
    virtual void draw()
    {
        /*CC_NODE_DRAW_SETUP();

        ccGLBlendFunc( m_sBlendFunc.src, m_sBlendFunc.dst );

        ccGLBindTexture2D( m_pobTexture->getName() );
        ccGLEnableVertexAttribs( kCCVertexAttribFlag_PosColorTex );

        #define kQuadSize sizeof(m_sQuad.bl)
        long offset = (long)&m_sQuad;

        // vertex
        int diff = offsetof( ccV3F_C4B_T2F, vertices);
        glVertexAttribPointer(kCCVertexAttrib_Position, 3, GL_FLOAT, GL_FALSE, kQuadSize, (void*) (offset + diff));

        // texCoods
        diff = offsetof( ccV3F_C4B_T2F, texCoords);
        glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, kQuadSize, (void*)(offset + diff));

        // color
        diff = offsetof( ccV3F_C4B_T2F, colors);
        glVertexAttribPointer(kCCVertexAttrib_Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, kQuadSize, (void*)(offset + diff));


        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);* /

        DrawCommand cmd;
        kmGLGetMatrix(KM_GL_MODELVIEW, &cmd.mat);
        cmd.blendFunc = m_sBlendFunc;
        cmd.shaderProgram = getShaderProgram()->getProgram();
        cmd.textureId = m_pobTexture->getName();
        cmd.quad = m_sQuad;

        getRenderer()->addCommand(cmd);
    }
};*/