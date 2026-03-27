#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/CCEGLView.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <windows.h>
#include "VulkanRenderer.hpp"

using namespace geode::prelude;

VulkanRenderer* getRenderer()
{
    static VulkanRenderer* instance = nullptr;

    if (!instance)
        instance = new VulkanRenderer();

    return instance;
}

class $modify (CCEGLView)
{
    virtual void swapBuffers()
    {
        static CCLabelBMFont* lbl = []{
            auto lbl = CCLabelBMFont::create("", "bigFont.fnt");
            CCPoolManager::sharedPoolManager()->removeObject(lbl);
            lbl->setAnchorPoint(ccp(0, 0));
            lbl->setScale(0.5f);
            lbl->setPosition(ccp(30, 30));

            return lbl;
        }();

        getRenderer()->end();

        lbl->setString(fmt::format("VK Draw Calls: {}", drawCalls).c_str());
        lbl->visit();
        getRenderer()->present();
    }
};

class $modify (CCDirector)
{
    void setOpenGLView(CCEGLView *pobOpenGLView)
    {
        CCDirector::setOpenGLView(pobOpenGLView);

        static bool init = false;

        if (!init)
        {
            getRenderer()->init();
            init = true;
        }
    }

    void drawScene(void)
    {
        if (!getRenderer()->begin())
            return;

        CCDirector::drawScene();
    }
};